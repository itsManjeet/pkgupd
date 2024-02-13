#ifndef PKGUPD_COMMON_HH
#define PKGUPD_COMMON_HH

#include "../src/Configuration.h"
#include "../src/Engine.h"
#include <iostream>

static inline bool ask_user(const std::string& mesg, Configuration* config) {
    if (config->get("mode.ask", true)) {
        std::cout << mesg << " [Y|N] >> ";
        char c;
        std::cin >> c;
        if (c == 'Y' || c == 'y') {
            return true;
        } else {
            return false;
        }
    }
    return true;
}

#define PKGUPD_MODULE(id)                                                      \
    extern "C" int PKGUPD_##id(std::vector<std::string> const& args,           \
            Engine* engine, Configuration* config)

#define PKGUPD_MODULE_HELP(id)                                                 \
    extern "C" void PKGUPD_help_##id(std::ostream& os, int padding)
#define CHECK_ARGS(s)                                                          \
    if (args.size() != s) {                                                    \
        std::cerr << "need exactly " << s << " arguments" << std::endl;        \
        return 1;                                                              \
    }
#define PADDING std::string(padding, ' ')

#endif
