#include "../libpkgupd/downloader.hh"
using namespace rlxos::libpkgupd;

#include <iostream>
using namespace std;

PKGUPD_MODULE_HELP(sync) { os << "sync repository data to local" << endl; }

PKGUPD_MODULE(sync) {
  auto downloader = std::make_shared<Downloader>(config);
  filesystem::path repo_dir =
      config->get<std::string>(DIR_REPO, DEFAULT_REPO_DIR);
  if (!filesystem::exists(repo_dir)) {
    error_code err;
    filesystem::create_directories(repo_dir, err);
    if (err) {
      cout << "failed to create repository dir " << err.message() << endl;
      return 1;
    }
  }
  std::vector<std::string> repos;
  config->get(REPOS, repos);
  bool status = true;
  for (auto const& repo : repos) {
    cout << "syncing " << repo << endl;
    if (!downloader->get((repo + "/meta").c_str(), (repo_dir / repo).c_str())) {
      status = false;
      cerr << "failed:" << downloader->error() << endl;
    }
  }
  return status ? 0 : 1;
}