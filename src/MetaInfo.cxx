#include "MetaInfo.hxx"

#include <format>

using namespace rlxos::libpkgupd;

void MetaInfo::update_from(const YAML::Node& node) {
    READ_VALUE(std::string, id);
    READ_VALUE(std::string, version);
    READ_VALUE(std::string, about);
    READ_LIST(std::string, depends);
    READ_LIST(std::string, backup);

    READ_VALUE(std::string, cache);

    OPTIONAL_VALUE(std::string, integration, "");
}

std::string MetaInfo::package_name() const {
    return std::format("{}-{}-{}.pkg", id, version, cache);
}


std::string MetaInfo::str() const {
    std::stringstream ss;
    ss << "id: " << id << "\n";

    ss << "version: " << version << "\n"
            << "about: " << about << "\n"
            << "cache: " << cache << "\n";

    if (!depends.empty()) {
        ss << "depends:"
                << "\n";
        for (auto const& i: depends) ss << " - " << i << "\n";
    }

    if (!backup.empty()) {
        ss << "backup:"
                << "\n";
        for (auto const& i: backup) ss << " - " << i << "\n";
    }

    if (!integration.empty()) {
        ss << "script: |\n";
        std::string line;
        while (std::getline(ss, line)) {
            ss << "  " << line << '\n';
        }
        ss << std::endl;
    }

    return ss.str();
}
