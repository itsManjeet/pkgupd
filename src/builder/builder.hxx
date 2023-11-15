#ifndef LIBPKGUPD_BUILDER
#define LIBPKGUPD_BUILDER

#include <filesystem>
#include <system_error>

#include "../archive-manager/archive-manager.hxx"
#include "../configuration.hxx"
#include "../defines.hxx"
#include "../repository.hxx"
#include "../system-database.hxx"
#include "../utils/utils.hxx"

namespace rlxos::libpkgupd {

#define BUILD_TYPE_LIST                \
    X(CMake, cmake, "CMakeLists.txt")  \
    X(Meson, meson, "meson.build")     \
    X(Script, script, "")              \
    X(AutoConf, autoconf, "configure") \
    X(PySetup, pysetup, "setup.py")    \
    X(Go, go, "go.mod")                \
    X(Cargo, cargo, "cargo.toml")      \
    X(Makefile, makefile, "Makefile")

    enum class BuildType : int {
#define X(ID, name, file) ID,
        BUILD_TYPE_LIST
#undef X
        N_BUILD_TYPE
    };

#define BUILD_TYPE_INT(i) static_cast<int>(i)

    static char const *BUILD_TYPE_STR[BUILD_TYPE_INT(BuildType::N_BUILD_TYPE)] = {
#define X(ID, name, file) #name,
            BUILD_TYPE_LIST
#undef X
    };

    static char const *BUILD_TYPE_FILE[BUILD_TYPE_INT(BuildType::N_BUILD_TYPE)] = {
#define X(ID, name, file) file,
            BUILD_TYPE_LIST
#undef X
    };

    static char const *BUILD_TYPE_NAME[BUILD_TYPE_INT(BuildType::N_BUILD_TYPE)] = {
#define X(ID, name, file) #name,
            BUILD_TYPE_LIST
#undef X
    };

#define BUILD_TYPE_FROM_STR(s)                                               \
    ({                                                                       \
        BuildType t = BuildType::N_BUILD_TYPE;                               \
        for (auto i = 0; i < BUILD_TYPE_INT(BuildType::N_BUILD_TYPE); i++) { \
            if (!strcmp(BUILD_TYPE_NAME[i], s)) {                            \
                t = BuildType(i);                                            \
                break;                                                       \
            }                                                                \
        }                                                                    \
        t;                                                                   \
    })

#define BUILD_TYPE_FROM_FILE(s)                                              \
    ({                                                                       \
        BuildType t = BuildType::N_BUILD_TYPE;                               \
        for (auto i = 0; i < BUILD_TYPE_INT(BuildType::N_BUILD_TYPE); i++) { \
            if (!strcmp(BUILD_TYPE_FILE[i], s)) {                            \
                t = BuildType(i);                                            \
                break;                                                       \
            }                                                                \
        }                                                                    \
        t;                                                                   \
    })

#define DETECT_BUILD_TYPE(path)                                                   \
    ({                                                                            \
        BuildType t = BuildType::N_BUILD_TYPE;                                    \
        for (std::string file :                                                   \
             {"CMakeLists.txt", "meson.build", "configure", "setup.py", "go.mod", \
              "Cargo.toml", "Makefile"}) {                                        \
            if (std::filesystem::exists(dir + "/" + file)) {                      \
                t = BUILD_TYPE_FROM_FILE(file.c_str());                           \
                break;                                                            \
            }                                                                     \
        }                                                                         \
        t;                                                                        \
    })

    class Recipe;

    class Builder;

    class Compiler : public Object {
    public:
        virtual bool compile(Recipe *recipe, Configuration *config,
                             std::string source_dir, std::string destdir,
                             std::vector<std::string> &environ) = 0;

        static std::shared_ptr<Compiler> create(BuildType buildType);
    };

    class Builder : public Object {
    private:
        Configuration *mConfig;
        std::string mBuildDir;
        std::string mSourceDir;
        std::string mPackageDir;

        std::vector<std::string> mPackages;

    public:
        Builder(Configuration *config) : mConfig{config} {
            mBuildDir = config->get<std::string>("dir.build",
                                                 "/tmp/pkgupd-" + utils::random(10));
            mSourceDir = config->get<std::string>(DIR_SRC, DEFAULT_SRC_DIR);
            mPackageDir = config->get<std::string>(DIR_PKGS, DEFAULT_PKGS_DIR);
        }

        ~Builder() {
            if (mConfig->get("build.clean", true)) {
                std::error_code err;
                if (std::filesystem::exists(mBuildDir)) {
                    std::filesystem::remove_all(mBuildDir, err);
                }
            }
        }

        bool prepare(std::vector<std::string> const &sources, std::string const &dir);

        std::vector<std::string> packages() {
            std::vector<std::string> build_packages = mPackages;
            mPackages.clear();
            return build_packages;
        }

        bool pack(
                std::vector<std::pair<std::shared_ptr<PackageInfo>, std::string>> const
                &dirs);

        bool compile(Recipe *recipe, std::string dir, std::string destdir,
                     std::vector<std::string> &environ);

        bool build(Recipe *recipe, SystemDatabase *systemDatabase,
                   Repository *repository);
    };
}  // namespace rlxos::libpkgupd

#endif