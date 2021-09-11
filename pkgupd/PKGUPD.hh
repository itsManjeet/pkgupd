#ifndef _PKGUPD_HH_
#define _PKGUPD_HH_

#include <string>
#include <vector>
#include <map>
#include <yaml-cpp/yaml.h>
#include "../libpkgupd/libpkgupd.hh"

namespace rlxos::libpkgupd
{

    class PKGUPD
    {
    public:
        enum class TaskType : int
        {
            INVLAID,
            INSTALL,
            COMPILE,
            REMOVE,
            REFRESH,
            UPDATE,
            DEPTEST,
            INFO
        };

        enum class FlagType : int
        {
            FORCE,
            SKIP_TRIGGER,
            SKIP_DEPENDS,
            NO_INSTALL,
        };

    private:
        TaskType task;
        std::map<std::string, FlagType> avaliableFlags{
            {"force", FlagType::FORCE},
            {"skip-triggers", FlagType::SKIP_TRIGGER},
            {"skip-depends", FlagType::SKIP_DEPENDS},
            {"no-install", FlagType::NO_INSTALL},
        };

        std::vector<FlagType> flags;
        std::vector<std::string> arguments;
        std::map<std::string, std::string> values;

        YAML::Node mConfigurations;

        std::shared_ptr<SystemDatabase> mSystemDatabase;
        std::shared_ptr<RepositoryDatabase> mRepositoryDatabase;
        std::shared_ptr<Downloader> mDownloader;
        std::shared_ptr<Installer> mInstaller;
        std::shared_ptr<Remover> mRemover;
        std::shared_ptr<Builder> mBuilder;

        std::shared_ptr<ResolveDepends> mResolveDepends;

        std::string SYS_DB = "sys-db";
        std::string REPO_DB = "repo-db";
        std::string PKG_DIR = "pkg-dir";
        std::string SRC_DIR = "src-dir";
        std::string ROOT_DIR = "root-dir";

        std::string configfile = "/etc/pkgupd.yml";

        void printHelp(char const *path);

        void parseArguments(int ac, char **av);

        bool checkAlteast(int size);
        bool checkSize(int size);

        std::string getValue(std::string var, std::string def);

        bool isFlag(FlagType f)
        {
            return (std::find(flags.begin(), flags.end(), f) != flags.end());
        }

    public:
        int Execute(int ac, char **av);
    };

}

#endif