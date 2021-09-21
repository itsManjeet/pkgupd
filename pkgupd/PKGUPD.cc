#include "PKGUPD.hh"

#include <bits/stdc++.h>
using namespace std;

namespace rlxos::libpkgupd
{

    bool PKGUPD::_need_atleast(int size)
    {
        if (_args.size() < size)
        {
            ERROR("Need alteast " << std::to_string(size) << " arguments");
            return false;
        }

        return true;
    }

    bool PKGUPD::_need_args(int size)
    {
        if (_args.size() != size)
        {
            ERROR("Need " << size << " arguments");
            return false;
        }

        return true;
    }
    void PKGUPD::_print_help(char const *path)
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

    void PKGUPD::_parse_args(int ac, char **av)
    {

        switch (av[1][0])
        {
        case 'i':
            !(strcmp(av[1], "in") && (strcmp(av[1], "install")))
                ? _task = task::INSTALL
                : (!(strcmp(av[1], "info"))
                       ? _task = task::INFO
                       : _task = task::INVLAID);
            break;
        case 'r':
            !(strcmp(av[1], "rm") && (strcmp(av[1], "remove")))
                ? _task = task::REMOVE
                : (!(strcmp(av[1], "rf") && strcmp(av[1], "refresh"))
                       ? _task = task::REFRESH
                       : _task = task::INVLAID);
            break;
        case 'u':
            !(strcmp(av[1], "up") && (strcmp(av[1], "update")))
                ? _task = task::UPDATE
                : _task = task::INVLAID;
            break;

        case 'd':
            !(strcmp(av[1], "deptest"))
                ? _task = task::DEPTEST
                : _task = task::INVLAID;
            break;

        case 'c':
            !(strcmp(av[1], "co") && (strcmp(av[1], "compile")))
                ? _task = task::COMPILE
                : _task = task::INVLAID;
            break;

        default:
            _task = task::INVLAID;
        }

        for (int i = 2; i < ac; i++)
        {
            string arg(av[i]);

            if (arg[0] == '-' && arg[1] == '-' && arg.length() > 2)
                if (_aval_flags.find(arg.substr(2, arg.length() - 2)) == _aval_flags.end())
                    throw std::runtime_error("invalid flag " + arg);
                else
                    _flags.push_back(_aval_flags[arg.substr(2, arg.length() - 2)]);
            else
            {
                size_t idx = arg.find_first_of('=');
                if (idx == string::npos)
                    _args.push_back(arg);
                else
                    _values[arg.substr(0, idx)] = arg.substr(idx + 1, arg.length() - (idx + 1));
            }
        }
    }

    string PKGUPD::_get_value(string var, string def)
    {
        if (_values.find(var) != _values.end())
            return _values[var];

        if (_config[var])
            return _config[var].as<std::string>();

        return def;
    }

    int PKGUPD::exec(int ac, char **av)
    {
        if (ac == 1)
        {
            _print_help(av[0]);
            return 0;
        }

        try
        {
            _parse_args(ac, av);
        }
        catch (std::exception const &ee)
        {
            cerr << ee.what() << endl;
            return 1;
        }

        if (_values.find("config") != _values.end())
        {
            if (!std::filesystem::exists(_values["config"]))
            {
                ERROR("Error! provided configuration file '" + _values["config"] + "' not exist");
                return 1;
            }

            _config_file = _values["config"];
        }
        else
        {
            for (auto const &i : {"pkgupd.yml", "/etc/pkgupd.yml"})
            {
                if (std::filesystem::exists(i))
                {
                    _config_file = i;
                    break;
                }
            }
        }

        if (std::filesystem::exists(_config_file))
            _config = YAML::LoadFile(_config_file);

        if (_config["environ"])
        {
            for (auto const &e : _config["environ"])
            {
                DEBUG("exporting " << e.as<string>());
                auto env = e.as<string>();
                size_t idx = env.find_first_of("=");
                setenv(env.substr(0, idx).c_str(), env.substr(idx + 1, env.length() - (idx + 1)).c_str(), 1);
            }
        }

        auto sysdb_ = sysdb(_get_value(SYS_DB, DEFAULT_DATA_DIR));
        auto repodb_ = repodb(_get_value(REPO_DB, DEFAULT_REPO_DIR));

        auto downloader_ = downloader();

        downloader_.urls({DEFAULT_URL, DEFAULT_SECONDARY_URL});

        auto installer_ = installer(sysdb_, repodb_, downloader_, _get_value(PKG_DIR, DEFAULT_PKGS_DIR));

        switch (_task)
        {
        case task::INSTALL:
        {
            if (!_need_atleast(1))
                return 1;

            std::vector<std::string> to_install;
            if (!_is_flag(flag::SKIP_DEPENDS))
            {
                PROCESS("resolving dependencies")
                for (auto const &i : _args)
                {
                    auto resolver_ = resolver(repodb_, sysdb_);
                    if (!resolver_.resolve(i))
                    {
                        ERROR(resolver_.error());
                        return 2;
                    }
                    to_install.insert(to_install.end(), resolver_.data().begin(), resolver_.data().end());
                }
            }
            else
            {
                to_install = _args;
            }

            if (!installer_.install(to_install, _get_value(ROOT_DIR, DEFAULT_ROOT_DIR), _is_flag(flag::SKIP_TRIGGER), _is_flag(flag::FORCE)))
            {
                ERROR(installer_.error());
                return 2;
            }

            return 0;
        }
        break;
        case task::COMPILE:
        {
            if (!_need_args(1))
                return 1;

            std::shared_ptr<recipe> recipe_;
            if (std::filesystem::exists(_args[0]))
                recipe_ = recipe::from_filepath(_args[0]);
            else
            {
                auto pkg = repodb_[_args[0]];
                if (pkg == nullptr)
                {
                    ERROR(repodb_.error());
                    return 2;
                }

                recipe_ = std::dynamic_pointer_cast<recipe::package>(pkg)->parent();
            }
            auto builder_ = builder(_get_value("work-dir", "/tmp"), _get_value(PKG_DIR, DEFAULT_PKGS_DIR), _get_value(SRC_DIR, DEFAULT_SRC_DIR), _is_flag(flag::FORCE));

            if (!builder_.build(recipe_))
            {
                ERROR(builder_.error());
                return 2;
            }
        }
        break;
        case task::INFO:
        {
            _need_args(1);

            std::shared_ptr<pkginfo> pkginfo_;
            pkginfo_ = sysdb_[_args[0]];
            if (pkginfo_ != nullptr)
            {
                auto sys_pkginfo = std::dynamic_pointer_cast<sysdb::package>(pkginfo_);
                cout << "id            : " << sys_pkginfo->id() << "\n"
                     << "version       : " << sys_pkginfo->version() << "\n"
                     << "about         : " << sys_pkginfo->about() << "\n"
                     << "installed on  : " << sys_pkginfo->installed_on() << "\n"
                     << "files         : " << sys_pkginfo->files().size() << endl;

                return 0;
            }

            pkginfo_ = repodb_[_args[0]];
            if (pkginfo_ == nullptr)
            {
                if (std::filesystem::exists(_args[0]))
                {
                    auto archive_ = archive(_args[0]);
                    pkginfo_ = archive_.info();
                    if (pkginfo_ == nullptr)
                    {
                        ERROR(archive_.error());
                        return 2;
                    }

                    auto archive_pkginfo = std::dynamic_pointer_cast<archive::package>(pkginfo_);
                    cout << "id            : " << archive_pkginfo->id() << "\n"
                         << "version       : " << archive_pkginfo->version() << "\n"
                         << "about         : " << archive_pkginfo->about() << endl;

                    return 0;
                }
                else
                {
                    ERROR(repodb_.error());
                    return 2;
                }
            }

            auto repo_pkginfo = std::dynamic_pointer_cast<recipe::package>(pkginfo_);
            cout << "id            : " << repo_pkginfo->id() << "\n"
                 << "version       : " << repo_pkginfo->version() << "\n"
                 << "about         : " << repo_pkginfo->about() << "\n"
                 << "provided by   : " << repo_pkginfo->parent()->id() << endl;

            return 0;
        }
        break;

        case task::REMOVE:
        {
            auto remover_ = remover(sysdb_, _get_value(ROOT_DIR, DEFAULT_ROOT_DIR));
            std::vector<std::string> _to_remove;
            for (auto const &i : _args)
            {
                if (sysdb_[i] != nullptr)
                    _to_remove.push_back(i);
                else
                    INFO(i << " is not already installed");
            }
            if (!remover_.remove(_to_remove, _is_flag(flag::SKIP_TRIGGER)))
            {
                ERROR(remover_.error());
                return 2;
            }

            return 0;
        }
        break;

        case task::DEPTEST:
        {
            std::vector<std::string> list;
            PROCESS("checking dependencies")
            for (auto const &i : _args)
            {
                auto resolver_ = resolver(repodb_, sysdb_);
                if (!resolver_.resolve(i))
                {
                    ERROR(resolver_.error());
                    return 2;
                }
                list.insert(list.end(), resolver_.data().begin(), resolver_.data().end());
            }

            for(auto const& i : list)
                std::cout << "-> " << i << std::endl;
        }
        break;

        case task::REFRESH:
        {
            PROCESS("refreshing repository");
            if (!downloader_.get("recipe", "/tmp/.rcp"))
            {
                ERROR(downloader_.error());
                return 2;
            }

            try
            {
                auto node = YAML::LoadFile("/tmp/.rcp");
                std::error_code ee;
                if (std::filesystem::exists(repodb_.data_dir()))
                {
                    std::filesystem::remove_all(repodb_.data_dir(), ee);
                    if (ee)
                    {
                        ERROR(ee.message());
                        return 2;
                    }

                    std::filesystem::create_directories(repodb_.data_dir(), ee);
                    if (ee)
                    {
                        ERROR(ee.message());
                        return 2;
                    }
                }

                for (auto const &i : node["recipes"])
                {

                    if (i["id"])
                    {
                        std::string id = i["id"].as<string>();
                        DEBUG("found " << id);
                        std::ofstream file(repodb_.data_dir() + "/" + id + ".yml");
                        if (!file.is_open())
                        {
                            ERROR("failed to open file in " + repodb_.data_dir() + " to write recipe file for " + id);
                            return 2;
                        }

                        file << i << std::endl;
                        file.close();
                    }
                }
            }
            catch (YAML::Exception const &ee)
            {
                ERROR(ee.what());
                return 2;
            }
        }
        }

        return 2;
    }
}