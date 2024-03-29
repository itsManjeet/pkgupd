#include "Configuration.h"

#include "Colors.h"
#include <filesystem>
#include <fstream>

static YAML::Node Merge(const YAML::Node& a, const YAML::Node& b) {
    if (a.IsNull())
        return b;
    else if (a.IsMap() && b.IsMap()) {
        YAML::Node merged = a;
        for (auto const& i : b) {
            auto key = i.first.as<std::string>();
            if (a[key]) {
                merged[key] = Merge(a[key], i.second);
            } else {
                merged[key] = i.second;
            }
        }
        return merged;
    } else if (a.IsSequence() && b.IsSequence()) {
        YAML::Node merged = a;
        for (const auto& elem : b) { merged.push_back(elem); }
        return merged;
    } else if (a.IsScalar() && b.IsScalar()) {
        return a;
    } else {
        std::stringstream ss;
        ss << a;
        throw std::runtime_error("Can't handle other type: " + ss.str());
    }
}

void Configuration::update_from_file(const std::string& filepath) {
    std::ifstream reader(filepath);
    if (!reader.good()) {
        throw std::runtime_error("failed to read file '" + filepath + "'");
    }
    std::string content((std::istreambuf_iterator<char>(reader)),
            (std::istreambuf_iterator<char>()));
    update_from(content, filepath);
}

void Configuration::update_from(
        const std::string& data, const std::string& filepath) {
    auto new_node = YAML::Load(data);
    node = Merge(node, new_node);
    if (new_node["merge"]) {
        for (auto const& i : new_node["merge"]) {
            try {
                auto path = std::filesystem::path(filepath).parent_path() /
                            i.as<std::string>();
                if (std::filesystem::exists(path)) {
                    update_from_file(
                            std::filesystem::path(filepath).parent_path() /
                            i.as<std::string>());
                } else {
                    bool found = false;
                    for (auto const& p : search_path) {
                        if (std::filesystem::exists(p / i.as<std::string>())) {
                            update_from_file(p / i.as<std::string>());
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        throw std::runtime_error(
                                "missing required file to merge '" +
                                i.as<std::string>() + "'");
                    }
                }

            } catch (const std::exception& exception) {
                throw std::runtime_error("failed to load " + filepath +
                                         " because " + exception.what() +
                                         " to merge");
            }
        }
    }
}
