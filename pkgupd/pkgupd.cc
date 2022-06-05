#include "../libpkgupd/configuration.hh"
#include "../libpkgupd/defines.hh"
using namespace rlxos::libpkgupd;

#define PKGUPD_MODULES_LIST \
  X(info)                   \
  X(install)                \
  X(search)                 \
  X(depends)                \
  X(meta)                   \
  X(remove)                 \
  X(build)                  \
  X(trigger)                \
  X(sync)                   \
  X(update)                 \
  X(inject)

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

bool append_config(YAML::Node& node, const char* file) {
  try {
    auto config = YAML::LoadFile(file);
    for (auto const& i : config) {
      node[i.first] = i.second;
    }
  } catch (std::exception const& e) {
    cerr << "Error! in configuration file " << file << ", " << e.what() << endl;
    exit(EXIT_FAILURE);
  }
  return true;
}

void print_help(char const* id) {
  cout << id << " [TASK] <args>.." << endl
       << "PKGUPD is a system package manager for rlxos." << endl
       << "Perform system level package transcations like installation, "
          "upgradation and removal of packages.\n"
       << endl;

  cout << "Task:" << endl;
#define X(id)            \
  cout << #id << "\t\t"; \
  PKGUPD_help_##id(cout);
  PKGUPD_MODULES_LIST
#undef X
}

int main(int argc, char** argv) {
  if (argc == 1) {
    print_help(argv[0]);
    exit(EXIT_FAILURE);
  }

  vector<string> args;
  YAML::Node node;
  node["self.path"] = argv[0];
  node["debug"] = false;

  if (filesystem::exists("/etc/pkgupd.yml")) {
    append_config(node, "/etc/pkgupd.yml");
  }

  if (filesystem::exists("/etc/pkgupd.conf.d")) {
    for (auto const& i :
         std::filesystem::directory_iterator("/etc/pkgupd.conf.d")) {
      if (i.path().has_extension() && i.path().extension() == ".yml") {
        append_config(node, i.path().c_str());
      }
    }
  }

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
      if (var == "config") {
        append_config(node, val.c_str());
      }
    } else {
      args.push_back(arg);
    }
  }

  if (node["debug"].as<bool>()) {
    cout << "Configuration: " << node << endl;
  }
  if (!node["repos"])
    node["repos"] = std::vector<std::string>{"core", "extra", "apps"};

  if (!node["mirrors"])
    node["mirrors"] =
        std::vector<std::string>{"https://rlxos.dev/storage/stable"};

  Configuration config(node);
  try {
    return iter->second(args, &config);
  } catch (std::exception const& err) {
    cerr << "Error! failed to perfrom task, unhandled exception " << err.what()
         << endl;
    return 1;
  }
}