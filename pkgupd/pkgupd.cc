#include "../libpkgupd/configuration.hh"
#include "../libpkgupd/defines.hh"
using namespace rlxos::libpkgupd;

#include <string.h>

#define PKGUPD_MODULES_LIST \
  X(install)                \
  X(remove)                 \
  X(sync)                   \
  X(info)                   \
  X(search)                 \
  X(update)                 \
  X(depends)                \
  X(inject)                 \
  X(meta)                   \
  X(build)                  \
  X(trigger)                \
  X(revdep)                 \
  X(owner)                  \
  X(run)                    \
  X(cleanup)                \
  X(watchdog)               \
  X(autoremove)

#include <functional>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#define X(id) PKGUPD_MODULE(id);
PKGUPD_MODULES_LIST
#undef X

#define X(id) PKGUPD_MODULE_HELP(id);
PKGUPD_MODULES_LIST
#undef X

map<string, function<int(vector<string> const&, Configuration*)>> MODULES = {
#define X(id) {#id, PKGUPD_##id},
    PKGUPD_MODULES_LIST
#undef X
};

bool is_number(string s) {
  for (auto c : s) {
    if (!(isdigit(c) || c == '.')) {
      return false;
    }
  }
  return true;
}

bool is_bool(string s) { return s == "true" || s == "false"; }

void print_help(char const* id) {
  cout << id << " [TASK] <args>.." << endl
       << "PKGUPD is a system package manager for rlxos." << endl
       << "Perform system level package transcations like installation, "
          "upgradation and removal of packages.\n"
       << endl;
  int size = 10;
#define X(id) \
  if (strlen(#id) > size) size = strlen(#id);
  PKGUPD_MODULES_LIST
#undef X

  cout << "Task:" << endl;
#define X(id)                                                        \
  cout << " - " << BLUE(#id) << std::string(size - strlen(#id), ' ') \
       << "    ";                                                    \
  PKGUPD_help_##id(cout, 2 + size + 4);
  PKGUPD_MODULES_LIST
#undef X

  cout << "\nExit Status:\n"
       << "  0 if OK\n"
       << "  1 if process failed.\n"
       << "\n"
       << "Full documentation <https://docs.rlxos.dev/pkgupd>\n"
       << endl;
}

int main(int argc, char** argv) {
  if (argc == 1) {
    print_help(argv[0]);
    exit(EXIT_FAILURE);
  }

  vector<string> args;
  YAML::Node node;

  string task = argv[1];
  auto iter = MODULES.find(task);
  if (iter == MODULES.end()) {
    print_help(argv[0]);
    exit(EXIT_FAILURE);
  }

  for (int i = 2; i < argc; i++) {
    string arg = argv[i];
    auto idx = arg.find_first_of('=');
    if (idx != std::string::npos) {
      auto var = arg.substr(0, idx);
      auto val = arg.substr(idx + 1);
      if (is_number(val)) {
        node[var] = stod(val);
      } else if (is_bool(val)) {
        node[var] = val == "true";
      } else if (val.find(',') != string::npos) {
        std::stringstream ss(val);
        std::string s;
        node[var] = std::vector<std::string>();
        while (std::getline(ss, s, ',')) {
          node[var].push_back(s);
        }
      } else {
        node[var] = val;
      }
    } else {
      args.push_back(arg);
    }
  }

  auto config = Configuration::create(node);
  // try {
    return iter->second(args, config.get());
  // } catch (std::exception const& err) {
    // cerr << "Error! failed to perfrom task, unhandled exception " << err.what()
    //      << endl;
    return 1;
  // }
}