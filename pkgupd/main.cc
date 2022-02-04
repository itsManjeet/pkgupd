#include <bits/stdc++.h>
#include <yaml-cpp/node/parse.h>

#include <cstring>
#include <exception>
#include <optional>
#include <stdexcept>
#include <vector>
using namespace std;

#include "../libpkgupd/defines.hh"
#include "../libpkgupd/libpkgupd.hh"
using namespace rlxos;

enum class Task : int {
  Invalid,
  Install,
  Remove,
  Compile,
  Depends,
  Info,
  Update,
  Refresh,
  Trigger,
  Search,
};

Task getTask(const char *task) {
  if (!strcmp(task, "in") || !strcmp(task, "install")) {
    return Task::Install;
  } else if (!strcmp(task, "co") || !strcmp(task, "compile")) {
    return Task::Compile;
  } else if (!strcmp(task, "depends")) {
    return Task::Depends;
  } else if (!strcmp(task, "info")) {
    return Task::Info;
  } else if (!strcmp(task, "update") || !strcmp(task, "up")) {
    return Task::Update;
  } else if (!strcmp(task, "refresh")) {
    return Task::Refresh;
  } else if (!strcmp(task, "remove") || !strcmp(task, "rm")) {
    return Task::Remove;
  } else if (!strcmp(task, "trigger")) {
    return Task::Trigger;
  } else if (!strcmp(task, "search")) {
    return Task::Search;
  }

  return Task::Invalid;
}

enum class Flag : int {
  Invalid,
  ConfigFile,
  NoTriggers,
  NoDepends,
  Force,
  NoAsk,
};

Flag getFlag(const char *flag) {
  if (!strcmp(flag, "--config")) {
    return Flag::ConfigFile;
  } else if (!strcmp(flag, "--no-triggers")) {
    return Flag::NoTriggers;
  } else if (!strcmp(flag, "--force")) {
    return Flag::Force;
  } else if (!strcmp(flag, "--no-ask")) {
    return Flag::NoAsk;
  } else if (!strcmp(flag, "--no-depends")) {
    return Flag::NoDepends;
  }
  return Flag::Invalid;
}

void printHelp(const char *prog) {
  cout << "Usage: " << prog << " [TASK] [ARGS]... [PKGS]..\n"
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
          "  up,  update                  upgarde specified"
          "package(s) to their latest avaliable version\n"
          "  co,  compile                 try to compile specified package(s) "
          "from repository recipe files\n"
          "  depends                      perform dependencies test for "
          "specified package\n"
          "  search                       search package from matched query\n"
          "  info                         print information of specified "
          "package\n"
          "  trigger                      execute require triggers and create "
          "required users & groups\n"
          "\n"
          "To override default values simply pass argument as "
          "--config=config.yml\n"
          "Parameters:\n"
          "  SystemDatabase               specify system database of installed "
          "packages\n"
       << "  CachePath                    specify dynamic cache for path for\n"
       << "  RootDir                      override the default root directory "
          "path\n"
       << "  Version                      alter the release version of rlxos\n"
       << "  Mirrors                      list of mirrors to search packages\n"
       << "Exit Status:\n"
          "  0  if OK\n"
          "  1  if issue with input data provided.\n"
          "  2  if build to perform specified task.\n"
          "\n"
          "Full documentation <https://docs.rlxos.dev/pkgupd>\n"
       << endl;
}

int main(int argc, char **argv) {
  vector<Flag> flags;
  vector<string> args;
  optional<string> configFile;

  if (argc == 1) {
    printHelp(argv[0]);
    return 0;
  }

  Task task = getTask(argv[1]);
  if (task == Task::Invalid) {
    cerr << "Error! invalid task '" << argv[1] << "'" << endl;
    printHelp(argv[0]);
    return 1;
  }

  if (filesystem::exists("/etc/pkgupd.yml")) {
    configFile = "/etc/pkgupd.yml";
  }

  for (int i = 2; i < argc; i++) {
    if (argv[i][0] == '-') {
      Flag flag = getFlag(argv[i]);
      if (flag == Flag::Invalid) {
        cerr << "Error! invalid flag '" << argv[i] << "'" << endl;
        return 1;
      }
      if (flag == Flag::ConfigFile) {
        if (argc == i) {
          cerr << "Error! no configuration file specified" << endl;
          return 1;
        }
        configFile = argv[++i];
      }
      flags.push_back(flag);
    } else {
      args.push_back(argv[i]);
    }
  }

  auto hasFlag = [&](Flag flag) -> bool {
    return find(flags.begin(), flags.end(), flag) != flags.end();
  };

  string SystemDatabase = "/var/lib/pkgupd/data";
  string CachePath = "/var/cache/pkgupd";
  string RootDir = "/";
  string Version = "2200";

  vector<string> Mirrors = {"https://rlxos.cloudtb.online",
                            "https://apps.rlxos.dev"};

  if (configFile) {
    DEBUG("loading configuration file '" << *configFile << "'");
    try {
      YAML::Node config = YAML::LoadFile(*configFile);

      auto getConfig = [&](string id, string fallback) -> string {
        if (config[id]) {
          DEBUG("using '" << id << "' : " << config[id].as<string>());
          return config[id].as<string>();
        }
        return fallback;
      };

      SystemDatabase = getConfig("SystemDatabase", SystemDatabase);
      CachePath = getConfig("CachePath", CachePath);
      RootDir = getConfig("RootDir", RootDir);
      Version = getConfig("Version", Version);

      if (config["Mirrors"]) {
        Mirrors.clear();
        for (auto const &i : config["Mirrors"]) {
          DEBUG("got mirror " << i.as<string>());
          Mirrors.push_back(i.as<string>());
        }
      }

      if (config["EnvironmentVariables"]) {
        for (auto const &i : config["EnvironmentVariables"]) {
          auto ev = i.as<string>();
          auto idx = ev.find_first_of('=');
          if (idx == string::npos) {
            ERROR("invalid EnvironmentVariable: " << ev);
            continue;
          }

          DEBUG("setting " << ev);
          setenv(ev.substr(0, idx).c_str(),
                 ev.substr(idx + 1, ev.length() - (idx + 1)).c_str(), 1);
        }
      }
    } catch (exception const &exp) {
      ERROR("Error! invalid configuration file, " << exp.what());
      return 2;
    }
  }

  libpkgupd::Pkgupd pkgupd(SystemDatabase, CachePath, Mirrors, Version, RootDir,
                           hasFlag(Flag::Force), hasFlag(Flag::NoTriggers));

  auto doTask = [&](Task task, libpkgupd::Pkgupd &pkgupd,
                    vector<string> const &args) -> bool {
    auto check_atleast = [&](int count) {
      if (args.size() < count) {
        throw runtime_error("need atleast '" + to_string(count) +
                            "' arguments but " + to_string(args.size()));
      }
    };

    auto check_exact = [&](int count) {
      if (args.size() != count) {
        throw runtime_error("need '" + to_string(count) + "' arguments but " +
                            to_string(args.size()));
      }
    };

    auto check_ask = [&](string mesg) -> bool {
      if (hasFlag(Flag::NoAsk)) {
        return true;
      }
      cout << "[Y|N] press 'Y' to " << mesg << ": ";
      auto ch = cin.get();
      if (ch == 'Y' || ch == 'y') {
        return true;
      }
      ERROR("user terminated the request");
      return false;
    };

    switch (task) {
      case Task::Install: {
        check_atleast(1);
        vector<string> dependencies;
        if (hasFlag(Flag::NoDepends)) {
          dependencies = args;
        } else {
          PROCESS("calculating dependencies");
          vector<string> for_dependencies_resolv;
          for (auto const &i : args) {
            if (std::filesystem::exists(i) &&
                std::filesystem::path(i).has_extension() &&
                !std::filesystem::is_directory(i)) {
              dependencies.push_back(i);
            } else {
              for_dependencies_resolv.push_back(i);
            }
          }

          for_dependencies_resolv = pkgupd.depends(for_dependencies_resolv);
          for (auto const &i : for_dependencies_resolv) {
            if (find(dependencies.begin(), dependencies.end(), i) ==
                dependencies.end()) {
              dependencies.push_back(i);
            }
          }

          INFO("found " << dependencies.size() << " dependencies");
          for (auto const &i : dependencies) {
            cout << i << endl;
          }

          if (dependencies.size() > 1) {
            if (!check_ask("install dependencies")) {
              return false;
            }
          }
        }

        return pkgupd.install(dependencies);
      }
      case Task::Compile:
        check_exact(1);
        return pkgupd.build(args[0]);
      case Task::Depends: {
        check_atleast(1);
        auto dependencies = pkgupd.depends(args);
        for (auto const &i : dependencies) {
          cout << i << endl;
        }
        if (pkgupd.error().length() != 0) {
          cerr << "Error! " << pkgupd.error() << endl;
          return false;
        }
        return true;
      }
      case Task::Remove: {
        check_atleast(1);
        return pkgupd.remove(args);
      }
      case Task::Info: {
        check_exact(1);
        auto package = pkgupd.info(args[0]);
        if (!package) {
          return false;
        }

        package->dump(cout);
        return true;
      };
      case Task::Refresh:
        check_exact(0);
        return pkgupd.sync();

      case Task::Trigger: {
        auto all_packages = pkgupd.list(libpkgupd::ListType::Installed);
        vector<string> all_packages_name;
        for (auto const &i : all_packages) {
          all_packages_name.push_back(i.id());
        }

        return pkgupd.trigger(all_packages_name);
      };

      case Task::Update: {
        check_exact(0);
        auto update_informations = pkgupd.outdate();
        vector<string> outdated;

        if (update_informations.size() == 0) {
          cout << "System is upto date" << endl;
          return true;
        }

        cout << "Found " << update_informations.size() << endl;
        for (auto const &i : update_informations) {
          cout << i.previous.id() << " " << i.previous.version() << " -> "
               << i.updated.version() << endl;
          outdated.push_back(i.updated.id());
        }

        if (!check_ask("update system")) {
          return true;
        }

        return pkgupd.update(outdated);
      };

      case Task::Search: {
        check_exact(1);
        auto result = pkgupd.search(args[0]);
        cout << "Found " << result.size() << " result(s)" << endl;
        for (auto const &i : result) {
          cout << i.id() << " : " << i.about() << endl;
        }
        return true;
      } break;
      default:
        ERROR("invalid task");
        return false;
    }
  };

  try {
    if (!doTask(task, pkgupd, args)) {
      ERROR(pkgupd.error());
      return 2;
    }
  } catch (exception const &err) {
    ERROR(err.what());
    return 2;
  }

  return 0;
}