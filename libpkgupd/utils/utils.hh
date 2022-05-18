#ifndef LIBPKGUPD_UTILS_HH
#define LIBPKGUPD_UTILS_HH

#include <functional>
#include <memory>
#include <string>

using defer = std::shared_ptr<void>;

#define DEFER(f) std::shared_ptr<void>(nullptr, std::bind([&] { f }))
namespace rlxos::libpkgupd::utils {
std::string random(size_t size);

}  // namespace rlxos::libpkgupd::utils

#endif