#include "Repository.h"

#include "defines.hxx"
#include "json.h"
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

void Repository::load(std::filesystem::path p) {
    path = p;
    mPackages.clear();

    PROCESS("Reading Repository Database");
    DEBUG("LOCATION   " << path);
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("no repository database found");
    }
    std::ifstream reader(path);
    nlohmann::json node = nlohmann::json::parse(reader);
    int count = 0;
    for (auto const& pkg_node : node) {
        count++;
        try {
            auto meta_data = MetaInfo();
            meta_data.id = pkg_node["id"];
            meta_data.version = pkg_node["version"];
            meta_data.about = pkg_node["about"];
            meta_data.cache = pkg_node["cache"];
            if (pkg_node.contains("integration"))
                meta_data.integration = pkg_node["integration"];
            if (pkg_node.contains("depends")) {
                for (auto const& i : pkg_node["depends"]) {
                    meta_data.depends.push_back(i);
                }
            }
            if (pkg_node.contains("backup")) {
                for (auto const& i : pkg_node["backup"]) {
                    meta_data.depends.push_back(i);
                }
            }
            mPackages[meta_data.id] = meta_data;
        } catch (const std::exception& exception) {
            ERROR("invalid data for " << count << "th meta info, skipping");
        }
    }
    DEBUG("DATABASE SIZE " << mPackages.size());
}

std::optional<MetaInfo> Repository::get(const std::string& id) const {
    if (auto const iter = mPackages.find(id); iter != mPackages.end()) {
        return iter->second;
    }
    return std::nullopt;
}
