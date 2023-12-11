#ifndef LIBPKGUPD_CONFIGURATION_HH
#define LIBPKGUPD_CONFIGURATION_HH

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

struct Configuration {
    YAML::Node node;

    Configuration() = default;

    void update_from(const std::string &input);


    template<typename T>
    T get(const std::string &key, T fallback) const {
        if (node[key]) {
            return node[key].as<T>();
        }
        return fallback;
    }

    template<typename T>
    T get(const std::string &key) const {
        if (node[key]) {
            return node[key].as<T>();
        }
        throw std::runtime_error("missing required key '" + key + "'");
    }

    template<typename T>
    void set(const std::string &key, T value) {
        node[key] = value;
    }

    template<typename T>
    void push(const std::string &key, T value) {
        if (!node[key]) {
            node[key] = {};
        }
        node[key].push_back(value);
    }
};

#endif
