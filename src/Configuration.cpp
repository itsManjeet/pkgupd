#include "Configuration.h"

#include "Colors.h"
#include <filesystem>

static inline std::string trim(std::string str) {
    str.erase(str.find_last_not_of(' ') + 1); //suffixing spaces
    str.erase(0, str.find_first_not_of(' ')); //prefixing spaces
    return str;
}


void Configuration::update_from(const std::string &input) {
    auto new_node = YAML::Load(input);
    for (auto const &i: new_node) {
        node[i.first] = i.second;
    }
}
