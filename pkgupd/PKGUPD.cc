#include "PKGUPD.hh"

#include <bits/stdc++.h>
using namespace std;

namespace rlxos::libpkgupd
{

    bool PKGUPD::checkAlteast(int size)
    {
        if (arguments.size() < size)
        {
            ERROR("Need alteast " << std::to_string(size) << " arguments");
            return false;
        }

        return true;
    }

    bool PKGUPD::checkSize(int size)
    {
        if (arguments.size() != size)
        {
            ERROR("Need " << size << " arguments");
            return false;
        }

        return true;
    }
    void PKGUPD::printHelp(char const *path)
    {
        cout << "Usage: " << path << " [TASK] [ARGS]... [PKGS]..\n"
             << "PKGUPD is a system package manager for rlxos.\n"
                "Perfrom system level package transactions like installations, upgradations and removal.\n\n"
                "TASK:\n"
                "  in,  install                 download and install specified package(s) from repository into the system\n"
                "  rm,  remove                  remove specified package(s) from the system if already installed\n"
                "  rf,  refresh                 synchronize local data with repositories\n"
                "  up,  update                  upgarde specified package(s) to their latest avaliable version\n"
                "  co,  compile                 try to compile specified package(s) from repository recipe files\n"
                "  deptest                      perform dependencies test for specified package\n"
                "  info                         print information of specified package\n"
                "\n"
                "To override default values simply pass argument as VALUE_NAME=VALUE\n"
                "Avaliable Values:\n"
                "  config                       override default configuration files path\n"
                "  download-url                 override primary repository url\n"
                "  secondary-download-url       override secondary repository url\n"
             << "  " << SYS_DB << "                       override default system database\n"
             << "  " << REPO_DB << "                      override default repository database path\n"
             << "\n"
             << "Exit Status:\n"
                "  0  if OK\n"
                "  1  if issue with input data provided.\n"
                "\n"
                "Full documentation <https://docs.rlxos.dev/pkgupd>\n"
                "or local manual: man pkgupd"
             << endl;
    }

    void PKGUPD::parseArguments(int ac, char **av)
    {

        switch (av[1][0])
        {
        case 'i':
            !(strcmp(av[1], "in") && (strcmp(av[1], "install")))
                ? task = TaskType::INSTALL
                : (!(strcmp(av[1], "info"))
                       ? task = TaskType::INFO
                       : task = TaskType::INVLAID);
            break;
        case 'r':
            !(strcmp(av[1], "rm") && (strcmp(av[1], "remove")))
                ? task = TaskType::REMOVE
                : (!(strcmp(av[1], "rf") && strcmp(av[1], "refresh"))
                       ? task = TaskType::REFRESH
                       : task = TaskType::INVLAID);
            break;
        case 'u':
            !(strcmp(av[1], "up") && (strcmp(av[1], "update")))
                ? task = TaskType::UPDATE
                : task = TaskType::INVLAID;
            break;

        case 'd':
            !(strcmp(av[1], "deptest"))
                ? task = TaskType::DEPTEST
                : task = TaskType::INVLAID;
            break;

        case 'c':
            !(strcmp(av[1], "co") && (strcmp(av[1], "compile")))
                ? task = TaskType::COMPILE
                : task = TaskType::INVLAID;
            break;

        default:
            task = TaskType::INVLAID;
        }

        for (int i = 2; i < ac; i++)
        {
            string arg(av[i]);

            if (arg[0] == '-' && arg[1] == '-' && arg.length() > 2)
                if (avaliableFlags.find(arg.substr(2, arg.length() - 2)) == avaliableFlags.end())
                    throw std::runtime_error("invalid flag " + arg);
                else
                    flags.push_back(avaliableFlags[arg.substr(2, arg.length() - 2)]);
            else
            {
                size_t idx = arg.find_first_of('=');
                if (idx == string::npos)
                    arguments.push_back(arg);
                else
                    values[arg.substr(0, idx)] = arg.substr(idx + 1, arg.length() - (idx + 1));
            }
        }
    }

    string PKGUPD::getValue(string var, string def)
    {
        if (values.find(var) != values.end())
            return values[var];

        if (mConfigurations[var])
            return mConfigurations[var].as<string>();

        return def;
    }

    int PKGUPD::Execute(int ac, char **av)
    {
        if (ac == 1)
        {
            printHelp(av[0]);
            return 0;
        }

        try
        {
            parseArguments(ac, av);
        }
        catch (std::exception const &ee)
        {
            cerr << ee.what() << endl;
            return 1;
        }

        if (values.find("config") != values.end())
        {
            if (!std::filesystem::exists(values["config"]))
            {
                ERROR("Error! provided configuration file '" + values["config"] + "' not exist");
                return 1;
            }

            configfile = values["config"];
        }

        if (std::filesystem::exists(values["config"]))
            mConfigurations = YAML::LoadFile(configfile);

        mSystemDatabase = std::make_shared<SystemDatabase>(getValue(SYS_DB, DEFAULT_DATA_DIR));
        mRepositoryDatabase = std::make_shared<RepositoryDatabase>(getValue(REPO_DB, DEFAULT_REPO_DIR));

        mResolveDepends = std::make_shared<ResolveDepends>(mRepositoryDatabase);
        mResolveDepends->SetSkipper(
            [&](std::string const &pkgid) -> bool
            {
                return ((*this->mSystemDatabase)[pkgid] != nullptr);
            });

        mDownloader = std::make_shared<Downloader>();
        mDownloader->AddURL(getValue("download-url", DEFAULT_URL));
        mDownloader->AddURL(getValue("secondary-download-url", DEFAULT_SECONDARY_URL));

        mBuilder = std::make_shared<Builder>();
        mBuilder->SetPackageDir(getValue(PKG_DIR, DEFAULT_PKGS_DIR));
        mBuilder->SetWorkDir(getValue("work-dir", "/tmp"));

        mInstaller = std::make_shared<Installer>();
        mInstaller->SetRepositoryDatabase(mRepositoryDatabase);
        mInstaller->SetSystemDatabase(mSystemDatabase);
        mInstaller->SetDownloader(mDownloader);

        mInstaller->SetPackagesDir(getValue(PKG_DIR, DEFAULT_PKGS_DIR));

        switch (task)
        {
        case TaskType::INVLAID:
            ERROR("Invalid task: " << av[1]);
            printHelp(av[0]);
            return 1;

        case TaskType::COMPILE:
            if (!checkAlteast(1))
                return 1;

            {
                std::vector<std::shared_ptr<RecipePackageInfo>> packagesList;
                for (auto const &i : arguments)
                {
                    auto pkginfo = (*mRepositoryDatabase)[i];
                    if (pkginfo == nullptr)
                    {
                        ERROR(mRepositoryDatabase->Error());
                        return 1;
                    }

                    packagesList.push_back(std::dynamic_pointer_cast<RecipePackageInfo>(pkginfo));
                }

                mBuilder->SetPackages(packagesList);

                if (!mBuilder->Build())
                {
                    ERROR("[COMPILATION] " << mBuilder->Error());
                    return 1;
                }

                if (!isFlag(FlagType::NO_INSTALL))
                {
                    mInstaller->SetPackages(mBuilder->PackagesList());
                    if (!mInstaller->Install(getValue(ROOT_DIR, DEFAULT_ROOT_DIR)))
                    {
                        ERROR("[INSTALLATION] " << mInstaller->Error());
                        return 1;
                    }
                }

                return 0;
            }
            break;

        case TaskType::INSTALL:
            if (!checkAlteast(1))
                return 1;
            {
                std::vector<std::string> pkgs;
                if (!isFlag(FlagType::SKIP_DEPENDS))
                {
                    PROCESS("resolving dependencies");

                    for (auto const &i : arguments)
                    {
                        if (!mResolveDepends->Resolve(i))
                        {
                            ERROR(mResolveDepends->Error())
                            return 2;
                        }
                    }

                    auto depends = mResolveDepends->GetData();
                    if (depends.size())
                    {
                        INFO("Required Dependencies")
                        for (auto const &i : depends)
                            std::cout << i << " ";

                        std::cout << std::endl;
                    }
                }

                mInstaller->SetPackages(arguments);

                mInstaller->SetSkipTriggers(isFlag(FlagType::SKIP_TRIGGER));
                mInstaller->SetForced(isFlag(FlagType::FORCE));

                if (!mInstaller->Install(getValue(ROOT_DIR, DEFAULT_ROOT_DIR)))
                {
                    ERROR(mInstaller->Error());
                    return 2;
                }

                return 0;
            }
            break;
        case TaskType::DEPTEST:
            if (!checkSize(1))
                return 1;
            {
                PROCESS("calculating dependencies for " << arguments[0]);
                if (!mResolveDepends->Resolve(arguments[0]))
                {
                    ERROR(mResolveDepends->Error())
                    return 1;
                }

                for (auto const &i : mResolveDepends->GetData())
                {
                    cout << " -> " << i << std::endl;
                }
                return 0;
            }
            break;

        case TaskType::INFO:
            if (!checkSize(1))
                return 1;

            {
                auto packageInfo = (*mSystemDatabase)[arguments[0]];
                if (packageInfo == nullptr)
                {
                    packageInfo = (*mRepositoryDatabase)[arguments[0]];
                    if (packageInfo == nullptr)
                    {
                        ERROR(mRepositoryDatabase->Error())
                        return 2;
                    }
                }

                cout << " ID            :    " << packageInfo->ID() << std::endl
                     << " Version       :    " << packageInfo->Version() << std::endl
                     << " About         :    " << packageInfo->About() << std::endl;

                return 0;
            }
            break;
        }

        return 0;
    }
}