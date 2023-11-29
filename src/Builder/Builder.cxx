#include "Builder.hxx"

#include <cstring>
#include <sys/stat.h>

#include <fstream>
#include <vector>

#include "../ArchiveManager/ArchiveManager.hxx"
#include "Bundler.hxx"
#include "../colors.hxx"
#include "../downloader.hxx"
#include "../exec.hxx"
#include "../Installer/installer.hxx"
#include "../recipe.hxx"
#include "../resolver.hxx"
#include "Stripper.hxx"

namespace fs = std::filesystem;
using std::string;

namespace rlxos::libpkgupd {
    bool Builder::prepare(std::vector<std::string> const& sources,
                          std::string const& dir) {
        for (auto const& source: sources) {
            auto sourcefile = fs::path(source).filename().string();
            auto url = source;

            if (auto const index = source.find("::"); index != string::npos) {
                sourcefile = source.substr(0, index);
                url = source.substr(index + 2, source.length() - (index + 2));
            }

            auto sourcefile_Path = fs::path(mSourceDir) / sourcefile;

            auto downloader = Downloader(mConfig);
            if (!fs::exists(sourcefile_Path)) {
                PROCESS("GET " << url);
                downloader.download(url.c_str(), sourcefile_Path.c_str());
            }

            auto endswith = [](std::string const& fullstr, std::string const& ending) {
                if (fullstr.length() >= ending.length())
                    return 0 == fullstr.compare(fullstr.length() - ending.length(),
                                                ending.length(), ending);
                return false;
            };

            bool extracted = false;
            for (auto const& i: {
                     ".tar", ".gz", ".tgz", ".xz", ".txz", ".bzip2", ".bz",
                     ".bz2", ".lzma"
                 }) {
                if (endswith(sourcefile, i)) {
                    PROCESS("extracting " << sourcefile)
                    if (int const status = Executor::execute(
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

            if (fs::path(sourcefile_Path).extension() == ".zip") {
                if (auto const status = Executor::execute("unzip " + sourcefile_Path.string() +
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

                fs::copy(sourcefile_Path, fs::path(dir) / sourcefile_Path.filename(), err);
                if (err) {
                    p_Error = "failed to copy " + sourcefile_Path.string() + ", error " +
                              err.message();
                    return false;
                }
            }
        }

        return true;
    }

    bool Builder::build(const Recipe& recipe, SystemDatabase* systemDatabase,
                        Repository* repository) {
        auto srcdir = fs::path(mBuildDir) / "src";
        auto pkgdir = fs::path(mBuildDir) / "pkg" / recipe.id;

        for (const auto& dir:
             std::vector{srcdir, pkgdir.parent_path()}) {
            std::error_code err;
            DEBUG("creating required dir: " << dir)
            create_directories(dir, err);
            if (err) {
                p_Error = "failed to create required build directories " + err.message();
                return false;
            }
        }

        if (!mConfig->get("build.skip-prepare", false)) {
            if (!prepare(recipe.sources, srcdir)) {
                return false;
            }
        }

        auto environ = recipe.environ;
        environ.push_back("PKGUPD_SRCDIR=" + mSourceDir);
        environ.push_back("PKGUPD_PKGDIR=" + mPackageDir);
        environ.push_back("pkgupd_srcdir=" + srcdir.string());
        environ.push_back("pkgupd_pkgdir=" + pkgdir.string());
        environ.push_back("DESTDIR=" + pkgdir.string());
        mConfig->get("environ", environ);

        std::filesystem::path wrkdir;
        auto build_work_type =
                mConfig->get<std::string>("build.work.type", "default");
        if (build_work_type == "default") {
            if (recipe.config["build-dir"]) {
            }
            wrkdir = srcdir / recipe.config["build-dir"].as<std::string>();
            if (recipe.config["build-dir"].as<std::string>().empty() && !recipe.sources.empty()) {
                if (auto [status, output] = Executor::output(
                    "tar -taf " +
                    std::filesystem::path(recipe.sources[0]).filename().string() +
                    " | head -n1"
                    " | cut -d '/' -f1",
                    mSourceDir); status != 0 || output.empty()) {
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
        if (recipe.config["pre-script"]) {
            PROCESS("executing prescript")
            if (int status =
                        Executor::execute(recipe.config["pre-script"].as<std::string>(), wrkdir.string(), environ);
                status != 0) {
                p_Error = "prescript failed with exit code: " + std::to_string(status);
                return false;
            }
        }

        if (recipe.config["include"]) {
            std::vector<MetaInfo> packages;
            for (auto const& pkg: recipe.config["include"]) {
                auto includePackage = repository->get(pkg.as<std::string>());
                if (!includePackage) {
                    p_Error = "missing required package to be included '" + pkg.as<std::string>() + "'";
                    return false;
                }
                packages.push_back(*includePackage);
            }
            auto config = Configuration(*mConfig);
            std::string local_roots = pkgdir;

            std::string local_datapath =
                    pkgdir / (recipe.config["include-path"]
                                  ? recipe.config["include-path"].as<std::string>()
                                  : "usr/share/" + recipe.id + "/included");

            config.node()[DIR_ROOT] = local_roots;
            config.node()[DIR_DATA] = local_datapath;
            // config.node()["installer.depends"] =
            //         recipe->get<bool>("include-depends", true);
            // config.node()["installer.triggers"] =
            //         recipe->get<bool>("installer.triggers", false);
            for (auto const& i: {local_roots, local_datapath}) {
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
            installer.install(packages, repository, &local_database);
            INFO("added included packages");
            std::map<std::string, std::string> environmentPaths{
                {"PATH", "usr/bin"},
                {"LD_LIBRARY_PATH", "usr/lib"},
                {"GI_TYPELIB_PATH", "usr/lib/girepository-1.0"},
                {"XDG_DATA_DIRS", "usr/share"},
                {"PKG_CONFIG_PATH", "usr/lib/pkgconfig"}
            };
            if (recipe.config["include-environments"]) {
                for (auto const& i: recipe.config["include-environments"]) {
                    environmentPaths[i.first.as<std::string>()] =
                            pkgdir.string() + "/" + i.second.as<std::string>();
                }
            }
            for (auto const& i: environmentPaths) {
                auto env = getenv(i.first.c_str());
                auto iter = std::find_if(environ.begin(), environ.end(),
                                         [&i](std::string const& j) -> bool {
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

        // if (!compile(recipe, wrkdir, pkgdir)) {
        //     return false;
        // }

        // if (!recipe->postscript().empty()) {
        //     PROCESS("executing postscript")
        //     if (int status =
        //                 Executor::execute(recipe->postscript(), wrkdir.string(), environ);
        //         status != 0) {
        //         p_Error = "postscript failed with exit code: " + std::to_string(status);
        //         return false;
        //     }
        // }
        // for (auto const& i: std::filesystem::recursive_directory_iterator(pkgdir)) {
        //     if (i.is_regular_file() && i.path().filename().extension() == ".la") {
        //         DEBUG("removing " + i.path().string());
        //         std::filesystem::remove(i);
        //     }
        // }
        //
        // if (recipe->dostrip()) {
        //     Stripper stripper(recipe->skipStrip());
        //     PROCESS("stripping package");
        //     if (!stripper.strip(pkgdir)) {
        //         ERROR(stripper.error());
        //     }
        // }

        // std::ofstream file(pkgdir / "info");
        // recipe->package()->dump(file);
        // file.close();
        //
        //
        // if (recipe->node()["bundle"] && recipe->node()["bundle"].as<bool>()) {
        //     auto bunder = Bundler(pkgdir, "/");
        //     std::vector<std::string> exclude;
        //     for (auto const& lib: recipe->node()["exclude-libraries"]) {
        //         exclude.push_back(lib.as<std::string>());
        //     }
        //     mConfig->get("bindler.exclude", exclude);
        //     // TODO: add list of libraries to include and exclude
        //     if (!bunder.resolveLibraries(exclude)) {
        //         p_Error = "Failed to bundle libraries, " + bunder.error();
        //         return false;
        //     }
        // }
        //
        // std::vector<std::pair<std::shared_ptr<MetaInfo>, std::string>> packagesdir;
        // packagesdir.emplace_back(recipe->package(), pkgdir);
        //
        // //        for (auto const &split: recipe->splits()) {
        // //            std::error_code err;
        // //            auto splitdir_Path = path(mBuildDir) / "pkg" / split.into;
        // //            fs::create_directories(splitdir_Path, err);
        // //            if (err) {
        // //                p_Error = "failed to create split dir " + err.message();
        // //                return false;
        // //            }
        // //
        // //            for (auto const &file: split.files) {
        // //                auto srcfile_Path = pkgdir / file;
        // //                auto destfile_Path = splitdir_Path / file;
        // //                PROCESS("creating " << destfile_Path);
        // //                fs::create_directories(destfile_Path.parent_path(), err);
        // //                if (err) {
        // //                    p_Error = "failed to create required dir " + err.message();
        // //                    return false;
        // //                }
        // //
        // //                fs::copy(srcfile_Path, destfile_Path,
        // //                         fs::copy_options::copy_symlinks |
        // //                         fs::copy_options::overwrite_existing |
        // //                         fs::copy_options::recursive,
        // //                         err);
        // //                if (err) {
        // //                    p_Error = "failed to copy file " + file + " " + err.message();
        // //                    return false;
        // //                }
        // //
        // //                fs::remove_all(srcfile_Path, err);
        // //                if (err) {
        // //                    p_Error = "failed to clean split file " + file + " " + err.message();
        // //                    return false;
        // //                }
        // //            }
        // //
        // //            std::ofstream file(splitdir_Path / "info");
        // //            auto id = split.into;
        // //
        // //            auto splitPackageInfo = (*recipe)[id];
        // //            splitPackageInfo->dump(file);
        // //            file.close();
        // //
        // //            packagesdir.push_back({splitPackageInfo, splitdir_Path});
        // //        }
        //
        // if (!pack(packagesdir)) {
        //     return false;
        // }
        //
        // if (mConfig->get("build.install", true)) {
        //     auto installer = Installer(mConfig);
        //     std::vector<std::pair<std::string, std::shared_ptr<MetaInfo>>> installData;
        //     for (int i = 0; i < mPackages.size(); i++) {
        //         installData.emplace_back(mPackages[i], packagesdir[i].first);
        //     }
        //
        //     if (!installer.install(installData, systemDatabase)) {
        //         p_Error = installer.error();
        //         return false;
        //     }
        // }

        return true;
    }


    // bool Builder::compile(Recipe* recipe, const std::string& build_root, const std::string& install_root) {
    //     p_Error = "no compiler module provided";
    //     return false;
    // }

    bool Builder::pack(
        std::vector<std::pair<MetaInfo, std::string>> const
        & dirs) {
        mPackages.clear();
        for (auto const& [packageInfo, packagePath]: dirs) {
            auto packagefile_Path = std::filesystem::proximate(mPackageDir) / packageInfo.package_name();


            if (!fs::exists(packagefile_Path.parent_path())) {
                std::error_code error;
                fs::create_directories(packagefile_Path.parent_path(), error);
                if (error) {
                    p_Error = "failed to create require directory '" +
                              packagefile_Path.parent_path().string() + "', " +
                              error.message();
                    return false;
                }
            }
            ArchiveManager::compress(packagefile_Path.c_str(),
                                     packagePath); {
                std::ofstream writer(packagefile_Path.parent_path() /
                                     (packageInfo.id + ".meta"));
                if (!writer.is_open()) {
                    p_Error = "failed to open info meta file for writing";
                    return false;
                }
                writer << packageInfo.str();
            }


            if (mConfig->get<bool>("build.env", false)) {
                std::ofstream env_writer(packagefile_Path.parent_path() /
                                         (packageInfo.id + ".env"));
                if (!env_writer.is_open()) {
                    p_Error = "failed to open env writer";
                    return false;
                }

                auto repository = std::make_shared<Repository>(mConfig);
                auto resolver = Resolver<MetaInfo>(
                    DEFAULT_GET_PACKAE_FUNCTION,
                    [](const MetaInfo& package_info) -> bool { return false; },
                    DEFAULT_DEPENDS_FUNCTION);
                std::vector<MetaInfo> packages;
                if (!resolver.depends(packageInfo, packages)) {
                    p_Error = "failed to write dev environment " + resolver.error();
                    return false;
                }
                for (auto const& i: packages) {
                    env_writer << i.id << " " << i.version << std::endl;
                }
                env_writer.close();
            }

            mPackages.push_back(packagefile_Path);
        }
        return true;
    }
} // namespace rlxos::libpkgupd
