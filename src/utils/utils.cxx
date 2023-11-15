#include "utils.hxx"

#include <unistd.h>

#include <regex>

std::string rlxos::libpkgupd::utils::random(size_t size) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    srand((unsigned)time(0));
    std::string tmp_s;
    tmp_s.reserve(size);

    for (int i = 0; i < size; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return tmp_s;
}

static std::string replace_all(std::string str, const std::string &from, const std::string &to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();  // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

void rlxos::libpkgupd::utils::resolve_variable(YAML::Node node,
                                               std::string &value) {
    std::vector<std::string> variables = {"id", "version", "release", "commit"};
    if (!node["variables"]) {
        node["variables"] = YAML::Node();
    }
    for (auto const &i : variables) {
        if (!node[i]) {
            node[i] = "";
        }
        node["variables"][i] = node[i];
    }
    for (auto const &var : node["variables"]) {
        variables.push_back(var.first.as<std::string>());
    }

    for (auto const &i : variables) {
        value = replace_all(value, "%{"+i+"}", node["variables"][i].as<std::string>());
    }
}