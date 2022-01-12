#include "PKGUPD.hh"

#include <bits/stdc++.h>
using namespace std;

namespace rlxos::libpkgupd {

bool PKGUPD::_need_atleast(int size) {
  if (_args.size() < size) {
    ERROR("Need alteast " << std::to_string(size) << " arguments");
    return false;
  }

  return true;
}

bool PKGUPD::_need_args(int size) {
  if (_args.size() != size) {
    ERROR("Need " << size << " arguments");
    return false;
  }

  return true;
}
void PKGUPD::_print_help(char const *path) {
  cout
      << "Usage: " << path << " [TASK] [ARGS]... [PKGS]..\n"
      << "PKGUPD is a system package manager for rlxos.\n"
         "Perfrom system level package transactions like installations, "
         "upgradations and removal.\n\n"
         "TASK:\n"
         "  in,  install                 download and install specified "
         "package(s) from repository into the system\n"
         "  rm,  remove                  remove specified package(s) from the "
         "system if already installed\n"
         "  rf,  refresh                 synchronize local data with "
         "repositories\n"
         "  sr,  search                  search package from repository\n"
         "  up,  update                  upgarde "
         "package(s) to their latest avaliable version\n"
         "  co,  compile                 try to compile specified package(s) "
         "from repository recipe files\n"
         "  deptest                      perform dependencies test for "
         "specified package\n"
         "  info                         print information of specified "
         "package\n"
         "  trigger                      execute require triggers and create "
         "required users & groups\n"
         "\n"
         "To override default values simply pass argument as VALUE_NAME=VALUE\n"
         "Avaliable Values:\n"
         "  config                       override default configuration files "
         "path\n"
      << "  " << SYS_DB
      << "                       override default system database\n"
      << "  " << REPO_DB
      << "                      override default repository database path\n"
      << "  " << PKG_DIR
      << "                      override default package directory path\n"
      << "  " << SRC_DIR
      << "                      override default source directory path\n"
      << "  " << ROOT_DIR
      << "                     override default root directory path\n"
      << "\n"
      << "Exit Status:\n"
         "  0  if OK\n"
         "  1  if issue with input data provided.\n"
         "\n"
         "Full documentation <https://docs.rlxos.dev/pkgupd>\n"
      << endl;
}

void PKGUPD::_parse_args(int ac, char **av) {
  switch (av[1][0]) {
    case 'i':
      !(strcmp(av[1], "in") && (strcmp(av[1], "install")))
          ? _task = task::INSTALL
          : (!(strcmp(av[1], "info")) ? _task = task::INFO
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
      !(strcmp(av[1], "deptest")) ? _task = task::DEPTEST
                                  : _task = task::INVLAID;
      break;

    case 'c':
      !(strcmp(av[1], "co") && (strcmp(av[1], "compile")))
          ? _task = task::COMPILE
          : _task = task::INVLAID;
      break;

    case 't':
      !(strcmp(av[1], "trigger")) ? _task = task::TRIGGERS
                                  : _task = task::INVLAID;
      break;

    case 's':
      !(strcmp(av[1], "sr") && (strcmp(av[1], "search")))
          ? _task = task::SEARCH
          : _task = task::INVLAID;
      break;

    default:
      _task = task::INVLAID;
  }

  for (int i = 2; i < ac; i++) {
    string arg(av[i]);

    if (arg[0] == '-' && arg[1] == '-' && arg.length() > 2)
      if (_aval_flags.find(arg.substr(2, arg.length() - 2)) ==
          _aval_flags.end())
        throw std::runtime_error("invalid flag " + arg);
      else
        _flags.push_back(_aval_flags[arg.substr(2, arg.length() - 2)]);
    else {
      size_t idx = arg.find_first_of('=');
      if (idx == string::npos)
        _args.push_back(arg);
      else
        _values[arg.substr(0, idx)] =
            arg.substr(idx + 1, arg.length() - (idx + 1));
    }
  }
}

string PKGUPD::_get_value(string var, string def) {
  if (_values.find(var) != _values.end()) return _values[var];

  if (_config[var]) return _config[var].as<std::string>();

  return def;
}

int PKGUPD::exec(int ac, char **av) {
  if (ac == 1) {
    _print_help(av[0]);
    return 0;
  }

  try {
    _parse_args(ac, av);
  } catch (std::exception const &ee) {
    cerr << ee.what() << endl;
    return 1;
  }

  if (_values.find("config") != _values.end()) {
    if (!std::filesystem::exists(_values["config"])) {
      ERROR("Error! provided configuration file '" + _values["config"] +
            "' not exist");
      return 1;
    }

    _config_file = _values["config"];
  } else {
    for (auto const &i : {"pkgupd.yml", "/etc/pkgupd.yml"}) {
      if (std::filesystem::exists(i)) {
        _config_file = i;
        break;
      }
    }
  }

  if (std::filesystem::exists(_config_file))
    _config = YAML::LoadFile(_config_file);

  if (_config["environ"]) {
    for (auto const &e : _config["environ"]) {
      DEBUG("exporting " << e.as<string>());
      auto env = e.as<string>();
      size_t idx = env.find_first_of("=");
      setenv(env.substr(0, idx).c_str(),
             env.substr(idx + 1, env.length() - (idx + 1)).c_str(), 1);
    }
  }

  std::vector<std::string> mirrors = {"https://rlxos.cloudtb.online/",
                                      "https://apps.rlxos.dev/"};
  if (_config["mirrors"]) {
    for (auto const &m : _config["mirrors"]) {
      mirrors.push_back(m.as<std::string>());
    }
  }

  Pkgupd pkgupd(_get_value(SYS_DB, DEFAULT_DATA_DIR),
                _get_value(REPO_DB, DEFAULT_REPO_DIR),
                _get_value(PKG_DIR, DEFAULT_PKGS_DIR),
                _get_value(SRC_DIR, DEFAULT_SRC_DIR),
                 mirrors, "2200",
                _get_value(ROOT_DIR, DEFAULT_ROOT_DIR), _is_flag(flag::FORCE),
                _is_flag(flag::SKIP_TRIGGER));

  switch (_task) {
    case task::UPDATE: {
      if (!pkgupd.sync()) {
        ERROR(pkgupd.error());
        return 2;
      }

      auto outdated = pkgupd.outdate();
      if (outdated.size() == 0) {
        INFO("system is upto date");
        return 0;
      }

      std::vector<std::string> packages;
      for (auto const &i : outdated) {
        std::cout << i.previous.id() << " (" << i.previous.version() << " -> "
                  << i.updated.version() << std::endl;
        packages.push_back(i.updated.id());
      }

      std::cout << "Found " << packages.size()
                << " packages(s) update, Press 'Y' to update" << std::endl;

      char ch;
      std::cin >> ch;
      if (ch != 'Y' && !_is_flag(flag::NOASK)) {
        std::cout << "User aborted the process" << std::endl;
        return 2;
      }

      if (!pkgupd.update(packages)) {
        ERROR(pkgupd.error());
        return 2;
      }

      return 0;
    } break;
    case task::SEARCH: {
      if (!_need_args(1)) return 1;
      std::string query = _args[0];

      auto packages = pkgupd.search(query);
      if (packages.size() == 0) {
        std::cout << "no package found" << std::endl;
        return 2;
      }

      for (auto const &i : packages) {
        std::cout << i.id() << " : " << i.about() << std::endl;
      }

      INFO("found " << packages.size() << " results");
      return 0;

    } break;
    case task::INSTALL: {
      if (!_need_atleast(1)) return 1;

      std::vector<std::string> to_install;

      if (_args.size() == 1 && std::filesystem::exists(_args[0])) {
        to_install = {_args[0]};
      } else if (!_is_flag(flag::SKIP_DEPENDS)) {
        DEBUG("resolving dependencies")
        for (auto const &i : _args) {
          to_install = pkgupd.depends(i);
          if (to_install.size() == 0 && pkgupd.error().size()) {
            ERROR(pkgupd.error());
            return false;
          }
        }
      } else {
        to_install = _args;
      }

      if (to_install.size() == 0) {
        INFO("dependencies already resolved");
        return 0;
      }

      if (!pkgupd.install(to_install)) {
        ERROR(pkgupd.error());
        return 2;
      }

      INFO("Done")
      return 0;
    } break;
    case task::INFO: {
      _need_args(1);

      auto package = pkgupd.info(_args[0]);
      if (!package) {
        ERROR(pkgupd.error());
        return 2;
      }

      package->dump(std::cout);

      return 0;
    } break;

    case task::REMOVE: {
      if (!pkgupd.remove(_args)) {
        ERROR(pkgupd.error());
        return 2;
      }
      return 0;
    } break;

    case task::DEPTEST: {
      _need_atleast(1);

      std::vector<std::string> list;
      for (auto const &i : _args) {
        list = pkgupd.depends(i);
        if (list.size() == 0 && pkgupd.error().size()) {
          ERROR(pkgupd.error());
          return 2;
        }
      }

      for (auto const &i : list) std::cout << i << std::endl;

      return 0;
    } break;

    case task::REFRESH: {
      PROCESS("refreshing repository");
      if (!pkgupd.sync()) {
        ERROR(pkgupd.error());
        return 2;
      }

      return 0;
    } break;

    case task::TRIGGERS: {
      if (!pkgupd.trigger(_args)) {
        ERROR(pkgupd.error());
        return 2;
      }
      return 0;
    } break;

    case task::COMPILE: {
      _need_args(1);
      if (!pkgupd.build(_args[0])) {
        ERROR(pkgupd.error());
        return 2;
      }

      return 0;

    }break;

    default:
      ERROR("invalid task ");
      _print_help(av[0]);
  }

  return 2;
}
}  // namespace rlxos::libpkgupd