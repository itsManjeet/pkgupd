#include "common.h"
#include <cstring>

#define PKGUPD_DEFAULT_CONFIG "/etc/pkgupd.yml"
#define PKGUPD_MODULES_LIST                                                    \
    X(ignite)                                                                  \
    X(sysroot)                                                                 \
    X(unlocked)

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
        if (!(isdigit(c) || c == '.')) { return false; }
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
#define X(id)                                                                  \
    if (strlen(#id) > size) size = strlen(#id);
    PKGUPD_MODULES_LIST
#undef X

    cout << "Task:" << endl;
#define X(id)                                                                  \
    cout << " - " << BLUE(#id) << std::string(size - strlen(#id), ' ')         \
         << "    ";                                                            \
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
    vector<string> args;
    Configuration configuration;
    std::optional<std::string> task;

    if (std::filesystem::exists(PKGUPD_DEFAULT_CONFIG)) {
        configuration.update_from_file(PKGUPD_DEFAULT_CONFIG);
    }

    for (int i = 1; i < argc; i++) {
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
            } else if (is_number(val)) {
                configuration.set(val, std::stod(val));
            } else if (is_bool(val)) {
                configuration.set(val, val == "true" ? true : false);
            } else {
                configuration.set(var, val);
            }

        } else if (!task.has_value()) {
            task = arg;
        } else {
            args.push_back(arg);
        }
    }

    if (!task.has_value()) {
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    auto iter = MODULES.find(*task);
    if (iter == MODULES.end()) {
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    try {
        return iter->second(args, &configuration);
    } catch (std::exception const& err) {
        ERROR(err.what());
        return 1;
    }
}
