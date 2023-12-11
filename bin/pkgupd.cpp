#include "common.h"

#include <cstring>

#define PKGUPD_MODULES_LIST \
  X(install)                \
  X(remove)                 \
  X(sync)                   \
  X(info)                   \
  X(search)                 \
  X(update)                 \
  X(depends)                \
  X(trigger)                \
  X(owner)                  \
  X(build)                  \
  X(cleanup)                \
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

map<string, function<int(vector<string> const &, Engine *, Configuration *)>> MODULES = {
#define X(id) {#id, PKGUPD_##id},
        PKGUPD_MODULES_LIST
#undef X
};

bool is_number(string s) {
    for (auto c: s) {
        if (!(isdigit(c) || c == '.')) {
            return false;
        }
    }
    return true;
}

bool is_bool(string s) { return s == "true" || s == "false"; }

void print_help(char const *id) {
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

int main(int argc, char **argv) {
    if (argc == 1) {
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    vector<string> args;
    Configuration configuration;
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
            if (val.find(',') != string::npos) {
                std::stringstream ss(val);
                for (std::string s; std::getline(ss, s, ',');) {
                    configuration.push(var, s);
                }
            } else {
                configuration.set(var, val);
            }

        } else {
            args.push_back(arg);
        }
    }

    for (auto const &s: configuration.node) {
        std::cout << s.first << ": " << s.second << std::endl;
    }


    try {
        auto engine = Engine(configuration);
        return iter->second(args, &engine, &configuration);
    }
    catch (std::exception const &err) {
        ERROR(err.what());
        return 1;
    }
}
