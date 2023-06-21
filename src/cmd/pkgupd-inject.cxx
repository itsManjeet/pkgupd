#include "../archive-manager/archive-manager.hxx"
#include "../downloader.hxx"
#include "../installer/installer.hxx"

using namespace rlxos::libpkgupd;

#include <iostream>

using namespace std;

PKGUPD_MODULE_HELP(inject) {
    os << "Inject package from url or filepath directly into system\n"
       << PADDING << "  - Can be used for rlxos .app files or bundle packages"
       << endl
       << endl;
}
PKGUPD_MODULE(inject) {
    CHECK_ARGS(1);
    std::shared_ptr<Installer> installer = std::make_shared<Installer>(config);
    std::shared_ptr<SystemDatabase> system_database =
            std::make_shared<SystemDatabase>(config);

    std::string uri = args[0];

    if (uri.find("https://", 0) == 0 || uri.find("http://", 0) == 0 ||
        uri.find("ftp://", 0) == 0) {
        Downloader downloader(config);
        std::string filename =
                "/tmp/" + std::filesystem::path(uri).filename().string();
        if (std::filesystem::exists(filename)) {
            INFO(filename << " already downloaded");
        } else {
            if (!downloader.download(uri.c_str(), filename.c_str())) {
                ERROR("failed to download '"
                              << std::filesystem::path(filename).filename() << "' '"
                              << downloader.error() << "'");
                return 1;
            }
        }
        uri = filename;
    }

    auto archiveManager = ArchiveManager::create(uri);
    if (archiveManager == nullptr) {
        ERROR("failed to get a valid archive manager for " << uri);
        return 1;
    }

    auto packageInfo = archiveManager->info(uri.c_str());
    if (packageInfo == nullptr) {
        ERROR("failed to read package information for " << uri);
        return 1;
    }

    std::vector<std::pair<std::string, std::shared_ptr<PackageInfo>>> packages;
    packages.push_back({uri, packageInfo});
    if (!installer->install(packages, system_database.get())) {
        ERROR(installer->error());
        return 1;
    }

    return 0;
}