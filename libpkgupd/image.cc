#include "image.hh"

#include <sys/stat.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>

namespace rlxos::libpkgupd {

std::tuple<int, std::string> image::getdata(std::string const& _filepath) {
    std::string filepath = _filepath;
    if (filepath.substr(0, 2) == "./") {
        filepath = filepath.substr(2, filepath.length() - 2);
    }

    auto [status, output] = exec().output(_pkgfile + " --appimage-extract " + filepath, "/tmp/");
    if (status != 0) {
        return {status, "failed to get data from " + _pkgfile};
    }

    std::ifstream file("/tmp/squashfs-root/" + filepath);
    if (!file.good()) {
        return {1, filepath + " file is missing"};
    }

    std::filesystem::remove_all("/tmp/squashfs-root");

    return {0, std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>())};
}

std::shared_ptr<image::package> image::info() {
    auto [status, content] = getdata("./info");
    if (status != 0) {
        _error = "failed to read package information " + std::to_string(status) + ", " + content;
        return nullptr;
    }

    DEBUG("info: " << content);
    YAML::Node data;

    try {
        data = YAML::Load(content);
    } catch (YAML::Exception const& e) {
        _error = "corrupt package data, " + std::string(e.what());
        return nullptr;
    }

    return std::make_shared<image::package>(data, _pkgfile);
}

std::vector<std::string> image::list() {
    return {"/apps/" + std::filesystem::path(_pkgfile).filename().string()};
}

bool image::compress(std::string const& srcdir, std::shared_ptr<pkginfo> const& info) {
    std::string pardir = std::filesystem::path(_pkgfile).parent_path();
    if (!std::filesystem::exists(pardir)) {
        std::error_code err;
        std::filesystem::create_directories(pardir, err);
        if (err) {
            _error = "failed to create " + pardir + ", " + err.message();
            return false;
        }
    }

    std::ofstream fileptr(srcdir + "/info");

    fileptr << "id: " << info->id() << "\n"
            << "version: " << info->version() << "\n"
            << "about: " << info->about() << "\n";

    if (info->depends(false).size()) {
        fileptr << "depends:"
                << "\n";
        for (auto const& i : info->depends(false))
            fileptr << " - " << i << "\n";
    }

    if (info->users().size()) {
        fileptr << "users: " << std::endl;
        for (auto const& i : info->users()) {
            i->print(fileptr);
        }
    }

    if (info->groups().size()) {
        fileptr << "groups: " << std::endl;
        for (auto const& i : info->groups()) {
            i->print(fileptr);
        }
    }

    if (info->install_script().size()) {
        fileptr << "install_script: | " << std::endl;
        std::stringstream ss(info->install_script());
        std::string line;
        while (std::getline(ss, line, '\n'))
            fileptr << "  " << line << std::endl;
    }

    fileptr.close();

    std::set<std::string> req_libs = _list_req(srcdir);
    std::filesystem::path libdir = std::filesystem::path(srcdir) / "lib";

    std::error_code err;
    std::filesystem::create_directories(libdir);
    if (err) {
        _error = err.message();
        return false;
    }

    for (auto const& i : req_libs) {
        if (std::filesystem::exists(libdir / std::filesystem::path(i).filename())) {
            continue;
        }
        if (!std::filesystem::exists(i)) {
            ERROR("Missing " << i);
            continue;
        }
        DEBUG("copying " << i);
        std::filesystem::copy_file(i, libdir / std::filesystem::path(i).filename());
    }

    std::shared_ptr<recipe::package> _pkg = std::dynamic_pointer_cast<recipe::package>(info);

    {
        DEBUG("writing AppRun")

        if (_pkg->parent()->node()["AppRun"]) {
            _app_run = _pkg->parent()->node()["AppRun"].as<std::string>();
        }

        std::ofstream app_run_fstream(srcdir + "/AppRun");
        app_run_fstream << _app_run << std::endl;
        app_run_fstream.close();

        if (int status = chmod((srcdir + "/AppRun").c_str(), 0755); status != 0) {
            ERROR("failed to set executable permission on AppRun");
            return false;
        }
    }

    {
        DEBUG("writing desktop file")
        if (_pkg->parent()->node()["Desktopfile"]) {
            _desktop_file = _pkg->parent()->node()["Desktopfile"].as<std::string>();
        } else if (std::filesystem::exists(srcdir + "/share/applications/" + _pkg->parent()->id() + ".desktop")) {
            std::ifstream file(srcdir + "/share/applications/" + _pkg->parent()->id() + ".desktop");
            _desktop_file = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        } else if (_pkg->parent()->node()["DesktopfilePath"]) {
            std::ifstream file(_pkg->parent()->node()["DesktopfilePath"].as<std::string>());
            _desktop_file = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        } else if (!std::filesystem::exists(srcdir + "/" + _pkg->parent()->id() + ".desktop")) {
            _error = "failed to get desktop file";
            return false;
        }

        std::ofstream app_desktop_fstream(srcdir + "/" + _pkg->parent()->id() + ".desktop");
        app_desktop_fstream << _desktop_file << std::endl;
        app_desktop_fstream.close();
    }

    {
        DEBUG("packing AppImage")
        int status = exec().execute("appimagetool --sign " + srcdir + " " + _pkgfile);
        if (status != 0) {
            _error = "failed to pack appimage";
            return false;
        }
    }

    return true;
}

bool image::extract(std::string const& outdir) {
    std::error_code err;
    std::filesystem::copy(_pkgfile, outdir + "/apps/" + std::filesystem::path(_pkgfile).filename().string(), err);
    if (err) {
        _error = err.message();
        return false;
    }

    return true;
}

image::image(std::string const& p) : archive(p) {
    _app_run =
        "#!/bin/sh\n"
        "HERE=\"$(dirname \"$(readlink -f \"${0}\")\")\"\n"
        "export UNION_PRELOAD=\"${HERE}\"\n"
        "export LD_PRELOAD=\"${HERE}/libunionpreload.so\"\n"
        "export PATH=\"${HERE}\"/usr/bin/:\"${HERE}\"/usr/sbin/:\"${HERE}\"/usr/games/:\"${HERE}\"/bin/:\"${HERE}\"/sbin/:\"${PATH}\"\n"
        "export LD_LIBRARY_PATH=\"${HERE}\"/usr/lib/:\"${HERE}\"/usr/lib/i386-linux-gnu/:\"${HERE}\"/usr/lib/x86_64-linux-gnu/:\"${HERE}\"/usr/lib32/:\"${HERE}\"/usr/lib64/:\"${HERE}\"/lib/:\"${HERE}\"/lib/i386-linux-gnu/:\"${HERE}\"/lib/x86_64-linux-gnu/:\"${HERE}\"/lib32/:\"${HERE}\"/lib64/:\"${LD_LIBRARY_PATH}\"\n"
        "export PYTHONPATH=\"${HERE}\"/usr/share/pyshared/:\"${PYTHONPATH}\"\n"
        "export PYTHONHOME=\"${HERE}\"/usr/\n"
        "export XDG_DATA_DIRS=\"${HERE}\"/usr/share/:\"${XDG_DATA_DIRS}\"\n"
        "export PERLLIB=\"${HERE}\"/usr/share/perl5/:\"${HERE}\"/usr/lib/perl5/:\"${PERLLIB}\"\n"
        "export GSETTINGS_SCHEMA_DIR=\"${HERE}\"/usr/share/glib-2.0/schemas/:\"${GSETTINGS_SCHEMA_DIR}\"\n"
        "export QT_PLUGIN_PATH=\"${HERE}\"/usr/lib/qt4/plugins/:\"${HERE}\"/usr/lib/i386-linux-gnu/qt4/plugins/:\"${HERE}\"/usr/lib/x86_64-linux-gnu/qt4/plugins/:\"${HERE}\"/usr/lib32/qt4/plugins/:\"${HERE}\"/usr/lib64/qt4/plugins/:\"${HERE}\"/usr/lib/qt5/plugins/:\"${HERE}\"/usr/lib/i386-linux-gnu/qt5/plugins/:\"${HERE}\"/usr/lib/x86_64-linux-gnu/qt5/plugins/:\"${HERE}\"/usr/lib32/qt5/plugins/:\"${HERE}\"/usr/lib64/qt5/plugins/:\"${QT_PLUGIN_PATH}\"\n"
        "EXEC=$(grep -e '^Exec=.*' \"${HERE}\"/*.desktop | head -n 1 | cut -d \"=\" -f 2- | sed -e 's|%.||g')\n"
        "exec ${EXEC} \"$@\"";
}

std::string image::_mimetype(std::string const& path) {
    auto [status, output] = exec().output("file --mime-type " + path + " | awk '{print $2}'");
    if (status != 0) {
        return "";
    }

    return output.substr(0, output.size() - 1);
}

std::set<std::string> image::_list_lib(std::string const& path) {
    std::string lib_env;
    if (_libpath.size()) {
        lib_env = "LD_LIBRARY_PATH=";
        std::string sep;
        for (auto const& i : _libpath) {
            lib_env += sep + i;
            sep = ":";
        }
    }

    auto [status, output] = exec().output(lib_env + " ldd " + path + " | awk '{print $3}'");
    if (status != 0) {
        return {};
    }

    std::stringstream ss(output);
    std::set<std::string> list;
    std::string lib;
    while (ss >> lib) {
        list.insert(lib);
    }

    return list;
}

std::set<std::string> image::_list_req(std::string const& appdir) {
    std::set<std::string> list;

    for (auto const& d : std::filesystem::recursive_directory_iterator(appdir)) {
        if (d.is_directory()) {
            continue;
        }

        std::string _mime = _mimetype(d.path());
        if (_mime == "application/x-executable" ||
            _mime == "application/x-sharedlib") {
            std::set<std::string> _loc_list = _list_lib(d.path());
            list.insert(_loc_list.begin(), _loc_list.end());
        }
    }

    return list;
}

}  // namespace rlxos::libpkgupd
