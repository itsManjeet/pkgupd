#ifndef LIBPKGUPD_UTILS_HH
#define LIBPKGUPD_UTILS_HH

#include <yaml-cpp/yaml.h>

#include <functional>
#include <memory>
#include <string>

#define _R(var) rlxos::libpkgupd::utils::resolve_variable(data, var)
#define _RL(var) \
    for (auto &i : var) _R(i)

using defer = std::shared_ptr<void>;

#define DEFER(f) std::shared_ptr<void>(nullptr, std::bind([&] { f }))
namespace rlxos::libpkgupd::utils {
    std::string random(size_t size);

    static inline int get_version(std::string version) {
        version.erase(
            std::remove_if(version.begin(), version.end(),
                           [](auto const &c) -> bool { return !std::isdigit(c); }),
            version.end());
        if (version.length() == 0) {
            return 0;
        }
        return std::stoi(version);
    }

    void resolve_variable(YAML::Node node, std::string &value);
}  // namespace rlxos::libpkgupd::utils

#endif