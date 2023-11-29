#ifndef LIBPKGUPD_RECIPE
#define LIBPKGUPD_RECIPE

#include <yaml-cpp/yaml.h>

#include "defines.hxx"
#include "MetaInfo.hxx"

namespace rlxos::libpkgupd {
    struct Recipe : MetaInfo {
        std::vector<std::string> sources,
                build_depends,
                environ;
        YAML::Node config;

        Recipe() = default;

        Recipe(const YAML::Node& node);

        std::string hash() const;
    };
} // namespace rlxos::libpkgupd

#endif
