#include "../ArchiveManager/ArchiveManager.hxx"
#include "../configuration.hxx"
#include "../utils/utils.hxx"

using namespace rlxos::libpkgupd;

#include <fstream>

using namespace std;

PKGUPD_MODULE_HELP(meta) {
    os << "Generate meta information for package repository" << endl;
}

PKGUPD_MODULE(meta) {
    auto pkgs_dir = std::filesystem::path(
        config->get<std::string>(DIR_PKGS, DEFAULT_PKGS_DIR));
    std::vector<std::string> repos;
    config->get(REPOS, repos);

    if (repos.size() == 0) {
        ERROR("no repositories specified");
        return 1;
    }

    for (auto const &i : repos) {
        PROCESS("Generating meta information for " << GREEN(i));
        auto repo_path = pkgs_dir / i;
        if (!filesystem::exists(repo_path)) {
            ERROR("Error! '" << repo_path << "' not exists for " << i);
            return 1;
        }
        std::string datafile = config->get<std::string>("server.stability", "stable");
        DEBUG("STABILITY " << datafile);
        std::ofstream file(repo_path / datafile);
        if (!file.is_open()) {
            ERROR("failed to open " << repo_path);
            return 1;
        }
        YAML::Node parent;
        parent["pkgs"] = std::vector<YAML::Node>();
        for (auto const &pkg : filesystem::directory_iterator(repo_path)) {
            if (!(pkg.path().has_extension() && pkg.path().extension() == ".meta")) {
                continue;
            }
            std::string packagePath = pkg.path().string();
            try {
                YAML::Node node = YAML::LoadFile(packagePath);
                INFO("adding " << node["id"] << " " << node["version"]);
                parent["pkgs"].push_back(node);
            } catch (std::exception const &exc) {
                ERROR("failed to read data for " + packagePath + ", " +
                      std::string(exc.what()));
                continue;
            }
        }
        file << parent;
        file.close();
    }

    return 0;
}