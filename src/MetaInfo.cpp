#include "MetaInfo.h"

#include <format>

#include "Configuration.h"


void MetaInfo::update_from(const std::string &input) {
    config.update_from(input);
    id = config.get<std::string>("id");
    version = config.get<std::string>("version");
    about = config.get<std::string>("about");
    cache = config.get<std::string>("cache");

    if (config.node["depends"]) {
        for (auto const &dep: config.node["depends"]) {
            auto depend = dep.as<std::string>();
            if (depend.ends_with(".yml")) depend = depend.substr(0, depend.length() - 4);
            depends.emplace_back(depend);
        }
    }
    if (config.node["backup"]) {
        for (auto const &b: config.node["backup"]) backup.push_back(b.as<std::string>());
    }
    if (config.node["integration"]) {
        integration = config.node["integration"].as<std::string>();
    }

}

std::string MetaInfo::name() const {
    auto name = this->id;
    for (auto &c: name)
        if (c == '/') c = '-';
    return name;
}

std::string MetaInfo::package_name() const {
    return std::format("{}-{}-{}.pkg", name(), version, cache);
}


std::string MetaInfo::str() const {
    std::stringstream ss;
    ss << "id: " << id << "\n";

    ss << "version: " << version << "\n"
       << "about: " << about << "\n"
       << "cache: " << cache << "\n";

    if (!depends.empty()) {
        ss << "depends:\n";
        for (auto const &i: depends) ss << "- " << i << "\n";
    }

    if (!backup.empty()) {
        ss << "backup:\n";
        for (auto const &i: backup) ss << "- " << i << "\n";
    }

    if (!integration.empty()) {
        ss << "script: |-\n";
        std::string line;
        std::stringstream script(integration);
        while (std::getline(script, line)) {
            ss << "  " << line << '\n';
        }
        ss << std::endl;
    }

    return ss.str();
}
