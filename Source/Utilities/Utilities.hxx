#ifndef LIBPKGUPD_UTILS_HH
#define LIBPKGUPD_UTILS_HH

#include <yaml-cpp/yaml.h>

#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "../Configuration/Configuration.hxx"

#define _R(var) rlxos::libpkgupd::utils::resolve_variable(data, var)
#define _RL(var) \
  for (auto& i : var) _R(i)

using defer = std::shared_ptr<void>;

#define DEFER(f) std::shared_ptr<void>(nullptr, std::bind([&] { f }))
namespace libpkgupd::utils {
std::string random(size_t size);

static inline int get_version(std::string version) {
  version.erase(
      std::remove_if(version.begin(), version.end(),
                     [](auto const& c) -> bool { return !std::isdigit(c); }),
      version.end());
  if (version.length() == 0) {
    return 0;
  }
  return std::stoi(version);
}

void resolve_variable(YAML::Node node, std::string& value);

static inline bool ask_user(std::string mesg, Configuration* config) {
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

}  // namespace libpkgupd::utils

#endif