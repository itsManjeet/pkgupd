#include "recipe.hxx"
using namespace rlxos::libpkgupd;

#include <sstream>

#include "defines.hxx"
#include "utils/utils.hxx"


Recipe::Recipe(const YAML::Node& data)
    : config{data} {
    config["cache"] = "";
    auto const& node = config;

    update_from(node);

    READ_LIST(std::string, build_depends);
    READ_LIST(std::string, sources);
    READ_LIST(std::string, environ);
}

std::string Recipe::hash() const {
    std::stringstream ss;
    ss << config;

    unsigned h = 37;
    auto const str = ss.str();
    for (auto c: str)
        h = (h * 54059) ^ (str[0] * 76963);

    std::stringstream hex;
    hex << std::hex << h;
    return hex.str();
}
