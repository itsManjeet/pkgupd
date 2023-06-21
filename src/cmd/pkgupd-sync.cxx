#include "../downloader.hxx"

using namespace rlxos::libpkgupd;

#include <iostream>

using namespace std;

PKGUPD_MODULE_HELP(sync) {
    os << "Sync local database from server repository" << endl;
}

PKGUPD_MODULE(sync) {
    auto downloader = std::make_shared<Downloader>(config);
    filesystem::path repo_dir =
            config->get<std::string>(DIR_REPO, DEFAULT_REPO_DIR);
    if (!filesystem::exists(repo_dir)) {
        error_code err;
        filesystem::create_directories(repo_dir, err);
        if (err) {
            ERROR("failed to create repository dir " << err.message());
            return 1;
        }
    }
    std::vector<std::string> repos;
    config->get(REPOS, repos);
    if (repos.size() == 0) {
        ERROR("no repository specified, in configuration file");
        return 1;
    }
    bool status = true;
    for (auto const &repo: repos) {
        PROCESS("SYCING " << repo);
        std::string datafile =
                config->get<std::string>("server.stability", "stable");
        DEBUG("STABILITY " << datafile);
        if (!downloader->get((repo + "/" + datafile).c_str(),
                             (repo_dir / repo).c_str())) {
            status = false;
            ERROR(downloader->error());
        }
    }
    return status ? 0 : 1;
}