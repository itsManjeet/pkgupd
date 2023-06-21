#ifndef PKGUPD_COMMON_HH
#define PKGUPD_COMMON_HH

#include <iostream>

#include "configuration.hxx"

namespace rlxos::libpkgupd {
    static inline bool ask_user(std::string mesg, Configuration *config) {
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

}  // namespace rlxos::libpkgupd

#endif