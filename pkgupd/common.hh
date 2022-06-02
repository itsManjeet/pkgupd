#ifndef PKGUPD_COMMON_HH
#define PKGUPD_COMMON_HH

#include <iostream>

#include "../libpkgupd/configuration.hh"
namespace rlxos::libpkgupd {
static inline bool ask_user(std::string mesg, Configuration* config) {
  if (!config->get("mode.ask", false)) {
    std::cout << mesg << " [Y|N] >> ";
    int c = std::cin.get();
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