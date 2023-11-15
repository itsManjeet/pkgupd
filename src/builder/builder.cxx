#include "builder.hxx"

#include <string.h>
#include <sys/stat.h>

#include <fstream>
#include <vector>

#include "../archive-manager/archive-manager.hxx"
#include "bundler.hxx"
#include "../colors.hxx"
#include "../compilers/autoconf.hxx"
#include "../compilers/cargo.hxx"
#include "../compilers/cmake.hxx"
#include "../compilers/go.hxx"
#include "../compilers/makefile.hxx"
#include "../compilers/meson.hxx"
#include "../compilers/pysetup.hxx"
#include "../compilers/script.hxx"
#include "../downloader.hxx"
#include "../exec.hxx"
#include "../installer/installer.hxx"
#include "../recipe.hxx"
#include "../resolver.hxx"
#include "stripper.hxx"

namespace fs = std::filesystem;
using fs::path;
using std::string;
namespace rlxos::libpkgupd {

    static std::vector<std::string> HOT_FILES_LIST = {
#define X(ID, NAME, FILE) FILE,
            BUILD_TYPE_LIST
#undef X
    };

    bool Builder::prepare(std::vector<std::string> const &sources,
                          std::string const &dir) {
        for (auto const &source: sources) {
            auto sourcefile = path(source).filename().string();
            auto url = source;

            auto index = source.find("::");
            if (index != string::npos) {
                sourcefile = source.substr(0, index);
                url = source.substr(index + 2, source.length() - (index + 2));
            }

            auto sourcefile_Path = path(mSourceDir) / sourcefile;

            auto downloader = Downloader(mConfig);
            if (!fs::exists(sourcefile_Path)) {
                if (!downloader.download(url.c_str(), sourcefile_Path.c_str())) {
                    p_Error =
                            "failed to download '" + sourcefile + "' " + downloader.error();
                    return false;
                }
            }

            auto endswith = [](std::string const &fullstr, std::string const &ending) {
                if (fullstr.length() >= ending.length())
                    return (0 == fullstr.compare(fullstr.length() - ending.length(),
                                                 ending.length(), ending));
                else
                    return false;
            };

            bool extracted = false;
            for (auto const &i: {".tar", ".gz", ".tgz", ".xz", ".txz", ".bzip2", ".bz",
                                 ".bz2", ".lzma"}) {
                if (endswith(sourcefile, i)) {
                    PROCESS("extracting " << sourcefile)
                    if (int status = Executor().execute(
                                "tar -xPf " + sourcefile_Path.string() + " --no-same-owner -C " + dir);
                            status != 0) {
                        p_Error = "failed to extract " + sourcefile_Path.string() +
                                  " with tar, exit status: " + std::to_string(status);
                        return false;
                    }
                    extracted = true;
                    break;
                }
            }

            if (path(sourcefile_Path).extension() == ".zip") {
                if (int status = Executor().execute("unzip " + sourcefile_Path.string() +
                                                    " -d " + dir);
                        status != 0) {
                    p_Error = "failed to extract " + sourcefile_Path.string() +
                              " with unzip, exit status: " + std::to_string(status);
                    return false;
                }
                extracted = true;
            }

            if (!extracted) {
                std::error_code err;

                fs::copy(sourcefile_Path, path(dir) / sourcefile_Path.filename(), err);
                if (err) {
                    p_Error = "failed to copy " + sourcefile_Path.string() + ", error " +
                              err.message();
                    return false;
                }
            }
        }

        return true;
    }

    bool Builder::build(Recipe *recipe, SystemDatabase *systemDatabase,
                        Repository *repository) {
        auto srcdir = path(mBuildDir) / "src";
        auto pkgdir = path(mBuildDir) / "pkg" / recipe->id();

        for (auto dir:
                std::vector<std::filesystem::path>{srcdir, (pkgdir).parent_path()}) {
            std::error_code err;
            DEBUG("creating required dir: " << dir)
            fs::create_directories(dir, err);
            if (err) {
                p_Error = "failed to create required build directories " + err.message();
                return false;
            }
        }

        if (!mConfig->get("build.skip-prepare", false)) {
            if (!prepare(recipe->sources(), srcdir)) {
                return false;
            }
        }

        auto environ = recipe->environ();
        environ.push_back("PKGUPD_SRCDIR=" + mSourceDir);
        environ.push_back("PKGUPD_PKGDIR=" + mPackageDir);
        environ.push_back("pkgupd_srcdir=" + srcdir.string());
        environ.push_back("pkgupd_pkgdir=" + pkgdir.string());
        environ.push_back(
                "FILES_DIR=" +
                std::filesystem::path(recipe->filePath()).parent_path().string());
        environ.push_back("DESTDIR=" + pkgdir.string());
        mConfig->get("environ", environ);

        std::filesystem::path wrkdir;
        std::string build_work_type =
                mConfig->get<std::string>("build.work.type", "default");
        if (build_work_type == "default") {
            wrkdir = srcdir / recipe->buildDir();
            if (recipe->buildDir().length() == 0 && recipe->sources().size()) {
                auto [status, output] = Executor().output(
                        "tar -taf " +
                        std::filesystem::path(recipe->sources()[0]).filename().string() +
                        " | head -n1",
                        mSourceDir);
                if (status != 0 || output.length() == 0) {
                } else {
                    if (output[output.length() - 1] == '\n') {
                        output = output.substr(0, output.length() - 1);
                    }
                    wrkdir = srcdir / output;
                }
            }
        } else if (build_work_type == "local") {
            wrkdir =
                    std::filesystem::path(
                            mConfig->get<std::string>(
                                    "build.recipe", std::filesystem::current_path() / "recipe.yml"))
                            .parent_path();
        }
        if (recipe->prescript().size()) {
            PROCESS("executing prescript")
            if (int status =
                        Executor().execute(recipe->prescript(), wrkdir.string(), environ);
                    status != 0) {
                p_Error = "prescript failed with exit code: " + std::to_string(status);
                return false;
            }
        }

        if (recipe->include().size()) {
            std::vector<std::shared_ptr<PackageInfo>> packages;
            for (auto const &pkg: recipe->include()) {
                auto includePackage = repository->get(pkg.c_str());
                if (includePackage == nullptr) {
                    p_Error = "missing required package to be included '" + pkg + "'";
                    return false;
                }
                packages.push_back(includePackage);
            }
            auto config = Configuration(*mConfig);
            std::string local_roots = pkgdir;
            std::string local_datapath =
                    pkgdir / recipe->get<std::string>(
                            "include.path",
                            std::string("usr/share/") + recipe->id() + "/included");

            config.node()[DIR_ROOT] = local_roots;
            config.node()[DIR_DATA] = local_datapath;
            config.node()["installer.depends"] =
                    recipe->get<bool>("include-depends", true);
            config.node()["installer.triggers"] =
                    recipe->get<bool>("installer.triggers", false);
            for (auto const &i: {local_roots, local_datapath}) {
                if (!std::filesystem::exists(i)) {
                    std::error_code error;
                    std::filesystem::create_directories(i, error);
                    if (error) {
                        p_Error = "failed to create " + i + ", " + error.message();
                        return false;
                    }
                }
            }

            auto installer = Installer(&config);
            auto local_database = SystemDatabase(&config);
            if (!installer.install(packages, repository, &local_database)) {
                p_Error =
                        "failed to include specified packages '" + installer.error() + "'";
                return false;
            }
            INFO("added included packages");
            std::map<std::string, std::string> environmentPaths{
                    {"PATH",            "usr/bin"},
                    {"LD_LIBRARY_PATH", "usr/lib"},
                    {"GI_TYPELIB_PATH", "usr/lib/girepository-1.0"},
                    {"XDG_DATA_DIRS",   "usr/share"},
                    {"PKG_CONFIG_PATH", "usr/lib/pkgconfig"}};
            if (recipe->node()["include-environments"]) {
                for (auto const &i: recipe->node()["include-environments"]) {
                    environmentPaths[i.first.as<std::string>()] =
                            pkgdir.string() + "/" + i.second.as<std::string>();
                }
            }
            for (auto const &i: environmentPaths) {
                auto env = getenv(i.first.c_str());
                auto iter = std::find_if(environ.begin(), environ.end(),
                                         [&i](std::string const &j) -> bool {
                                             return j.find(i.first + "=", 0) == 0;
                                         });

                std::string envData =
                        i.second + (env == nullptr ? "" : ":" + std::string(env));
                if (iter != environ.end()) {
                    *iter = envData + ":" + (*iter);
                } else {
                    environ.push_back(i.first + "=" + envData);
                }
            }
        }

        PROCESS("compiling source code")

        if (!compile(recipe, wrkdir, pkgdir, environ)) {
            return false;
        }

        if (recipe->postscript().size()) {
            PROCESS("executing postscript")
            if (int status =
                        Executor().execute(recipe->postscript(), wrkdir.string(), environ);
                    status != 0) {
                p_Error = "postscript failed with exit code: " + std::to_string(status);
                return false;
            }
        }
        for (auto const &i: std::filesystem::recursive_directory_iterator(pkgdir)) {
            if (i.is_regular_file() && i.path().filename().extension() == ".la") {
                DEBUG("removing " + i.path().string());
                std::filesystem::remove(i);
            }
        }

        if (recipe->dostrip()) {
            Stripper stripper(recipe->skipStrip());
            PROCESS("stripping package");
            if (!stripper.strip(pkgdir)) {
                ERROR(stripper.error());
            }
        }

        std::ofstream file(pkgdir / "info");
        recipe->package()->dump(file);
        file.close();

        for (auto const &file: recipe->node()["files"]) {
            std::ofstream f(pkgdir / file["path"].as<string>());
            f << file["content"].as<string>();
            f.close();
            if (file["perm"]) {
                chmod((pkgdir / file["path"].as<string>()).c_str(),
                      file["perm"].as<int>());
            }
        }

        if (recipe->node()["bundle"] && recipe->node()["bundle"].as<bool>()) {
            Bundler bunder = Bundler(pkgdir, "/");
            std::vector<std::string> exclude;
            for (auto const &lib: recipe->node()["exclude-libraries"]) {
                exclude.push_back(lib.as<std::string>());
            }
            mConfig->get("bindler.exclude", exclude);
            // TODO: add list of libraries to include and exclude
            if (!bunder.resolveLibraries(exclude)) {
                p_Error = "Failed to bundle libraries, " + bunder.error();
                return false;
            }
        }

        std::vector<std::pair<std::shared_ptr<PackageInfo>, std::string>> packagesdir;
        packagesdir.push_back({recipe->package(), pkgdir});

//        for (auto const &split: recipe->splits()) {
//            std::error_code err;
//            auto splitdir_Path = path(mBuildDir) / "pkg" / split.into;
//            fs::create_directories(splitdir_Path, err);
//            if (err) {
//                p_Error = "failed to create split dir " + err.message();
//                return false;
//            }
//
//            for (auto const &file: split.files) {
//                auto srcfile_Path = pkgdir / file;
//                auto destfile_Path = splitdir_Path / file;
//                PROCESS("creating " << destfile_Path);
//                fs::create_directories(destfile_Path.parent_path(), err);
//                if (err) {
//                    p_Error = "failed to create required dir " + err.message();
//                    return false;
//                }
//
//                fs::copy(srcfile_Path, destfile_Path,
//                         fs::copy_options::copy_symlinks |
//                         fs::copy_options::overwrite_existing |
//                         fs::copy_options::recursive,
//                         err);
//                if (err) {
//                    p_Error = "failed to copy file " + file + " " + err.message();
//                    return false;
//                }
//
//                fs::remove_all(srcfile_Path, err);
//                if (err) {
//                    p_Error = "failed to clean split file " + file + " " + err.message();
//                    return false;
//                }
//            }
//
//            std::ofstream file(splitdir_Path / "info");
//            auto id = split.into;
//
//            auto splitPackageInfo = (*recipe)[id];
//            splitPackageInfo->dump(file);
//            file.close();
//
//            packagesdir.push_back({splitPackageInfo, splitdir_Path});
//        }

        if (!pack(packagesdir)) {
            return false;
        }

        if (mConfig->get("build.install", true)) {
            auto installer = Installer(mConfig);
            std::vector<std::pair<std::string, std::shared_ptr<PackageInfo>>> installData;
            for (int i = 0; i < mPackages.size(); i++) {
                installData.push_back({mPackages[i], packagesdir[i].first});
            }

            if (!installer.install(installData, systemDatabase)) {
                p_Error = installer.error();
                return false;
            }
        }

        return true;
    }

    std::shared_ptr<Compiler> Compiler::create(BuildType buildType) {
        switch (buildType) {
#define X(id, name, file) \
    case BuildType::id:   \
        return std::make_shared<id>();
            BUILD_TYPE_LIST
#undef X
        }
        return nullptr;
    }

    bool Builder::compile(Recipe *recipe, std::string dir, std::string destdir,
                          std::vector<std::string> &environ) {
        std::shared_ptr<Compiler> compiler;
        BuildType type;
        if (recipe->buildType() != BuildType::N_BUILD_TYPE) {
            type = recipe->buildType();
        } else if (recipe->script().size() != 0) {
            type = BuildType::Script;
        } else {
            DEBUG("detecting build type in: " << dir);
            type = DETECT_BUILD_TYPE(dir);
        }
        if (type == BuildType::N_BUILD_TYPE) {
            p_Error = "unable to detect build type inside " + dir;
            return false;
        }
        compiler = Compiler::create(type);
        if (compiler == nullptr) {
            p_Error = "unsupported compiler for type used '" +
                      std::string(BUILD_TYPE_STR[int(type)]) + "'";
            return false;
        }

        if (!compiler->compile(recipe, mConfig, dir.c_str(), destdir.c_str(),
                               environ)) {
            p_Error = compiler->error();
            return false;
        }

        return true;
    }

    bool Builder::pack(
            std::vector<std::pair<std::shared_ptr<PackageInfo>, std::string>> const
            &dirs) {
        mPackages.clear();
        for (auto const &i: dirs) {
            auto packagefile_Path = std::filesystem::proximate(mPackageDir) / (PACKAGE_FILE(i.first));

            auto archive_manager = ArchiveManager();

            if (!std::filesystem::exists(packagefile_Path.parent_path())) {
                std::error_code error;
                std::filesystem::create_directories(packagefile_Path.parent_path());
                if (error) {
                    p_Error = "failed to create require directory '" +
                              packagefile_Path.parent_path().string() + "', " +
                              error.message();
                    return false;
                }
            }
            if (!archive_manager.compress(packagefile_Path.c_str(),
                                          i.second.c_str())) {
                p_Error = "compression failed " + archive_manager.error();
                return false;
            }

            std::ofstream info_writer(packagefile_Path.parent_path() /
                                      (i.first->id() + ".meta"));
            if (!info_writer.is_open()) {
                p_Error = "failed to open info meta file for writing";
                return false;
            }
            i.first->dump(info_writer);
            info_writer.close();

            if (mConfig->get<bool>("build.env", false)) {
                std::ofstream env_writer(packagefile_Path.parent_path() /
                                         (i.first->id() + ".env"));
                if (!env_writer.is_open()) {
                    p_Error = "failed to open env writer";
                    return false;
                }

                auto repository = std::make_shared<Repository>(mConfig);
                auto resolver = Resolver<std::shared_ptr<PackageInfo>>(
                        DEFAULT_GET_PACKAE_FUNCTION,
                        [](std::shared_ptr<PackageInfo> package_info) -> bool { return false; },
                        DEFAULT_DEPENDS_FUNCTION);
                std::vector<std::shared_ptr<PackageInfo>> packages;
                if (!resolver.depends(i.first, packages)) {
                    p_Error = "failed to write dev environment " + resolver.error();
                    return false;
                }
                for (auto const &i: packages) {
                    env_writer << i->id() << " " << i->version() << std::endl;
                }
                env_writer.close();
            }

            mPackages.push_back(packagefile_Path);
        }
        return true;
    }

}  // namespace rlxos::libpkgupd
