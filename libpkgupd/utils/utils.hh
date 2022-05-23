#ifndef LIBPKGUPD_UTILS_HH
#define LIBPKGUPD_UTILS_HH

#include <functional>
#include <memory>
#include <string>

using defer = std::shared_ptr<void>;

#define DEFER(f) std::shared_ptr<void>(nullptr, std::bind([&] { f }))
namespace rlxos::libpkgupd::utils {
std::string random(size_t size);

static inline int get_version(std::string version) {
  version.erase(
      std::remove_if(version.begin(), version.end(),
                     [](auto const& c) -> bool { return !std::isalnum(c); }),
      version.end());
  if (version.length() == 0) {
    return 0;
  }
  return std::stoi(version);
}
}  // namespace rlxos::libpkgupd::utils

#endif