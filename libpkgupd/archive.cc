#include "archive.hh"

#include <fstream>

#include "exec.hh"

namespace rlxos::libpkgupd {

archive::package::package(YAML::Node const &data, std::string const &file) {
    READ_VALUE(std::string, id);
    READ_VALUE(std::string, version);
    READ_VALUE(std::string, about);
    READ_LIST(std::string, depends);

    READ_OBJECT_LIST(pkginfo::user, users);
    READ_OBJECT_LIST(pkginfo::group, groups);

    OPTIONAL_VALUE(std::string, install_script, "");
}

std::tuple<int, std::string> archive::getdata(std::string const &filepath) {
    std::string cmd = _archive_tool;
    cmd += " --zstd -O -xf " + _pkgfile + " " + filepath;

    auto [status, output] = exec().output(cmd);
    if (status != 0) {
        _error = "failed to get data from " + _pkgfile;
        return {status, output};
    }
    return {status, output};
}

std::shared_ptr<archive::package> archive::info() {
    auto [status, content] = getdata("./info");
    if (status != 0) {
        _error = "failed to read package information";
        return nullptr;
    }

    DEBUG("info: " << content);
    YAML::Node data;

    try {
        data = YAML::Load(content);
    } catch (YAML::Exception const &e) {
        _error = "corrupt package data, " + std::string(e.what());
        return nullptr;
    }

    return std::make_shared<archive::package>(data, _pkgfile);
}

std::vector<std::string> archive::list() {
    std::string cmd = _archive_tool;
    cmd += " --zstd -tf " + _pkgfile;

    auto [status, output] = exec().output(cmd);
    if (status != 0) {
        _error = output;
        return {};
    }

    std::stringstream ss(output);
    std::string file;

    std::vector<std::string> files_list;

    while (std::getline(ss, file, '\n'))
        files_list.push_back(file);

    return files_list;
}

bool archive::compress(std::string const &srcdir, std::shared_ptr<pkginfo> const &info) {
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
        for (auto const &i : info->depends(false))
            fileptr << " - " << i << "\n";
    }

    if (info->users().size()) {
        fileptr << "users: " << std::endl;
        for (auto const &i : info->users()) {
            i->print(fileptr);
        }
    }

    if (info->groups().size()) {
        fileptr << "groups: " << std::endl;
        for (auto const &i : info->groups()) {
            i->print(fileptr);
        }
    }

    if (info->install_script().size()) {
        fileptr << "install_script: | " << std::endl;
        std::stringstream ss(info->install_script());
        std::string line;
        while (std::getline(ss, line, '\n'))
            fileptr << "  " << line;
    }

    fileptr.close();

    std::string command = _archive_tool;

    command += " --zstd -cf " + _pkgfile + " -C " + srcdir + " . ";
    if (exec().execute(command) != 0) {
        _error = "failed to execute command for compression '" + command + "'";
        return false;
    }

    return true;
}

bool archive::extract(std::string const &outdir) {
    if (!std::filesystem::exists(_pkgfile)) {
        _error = "no " + _pkgfile + " exist";
        return false;
    }

    std::string cmd = _archive_tool;

    cmd += " --zstd --exclude './info' -xhpf " + _pkgfile + " -C " + outdir;
    if (exec().execute(cmd) != 0) {
        _error = "failed to execute extraction command";
        return false;
    }

    return true;
}
}  // namespace rlxos::libpkgupd