#include "../ArchiveManager/ArchiveManager.hxx"
#include "../downloader.hxx"
#include "../Installer/installer.hxx"

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
            downloader.download(uri.c_str(), filename.c_str());
        }
        uri = filename;
    }

    auto packageInfo = ArchiveManager::info(uri.c_str());


    std::vector<std::pair<std::string, MetaInfo>> packages;
    packages.push_back({uri, packageInfo});
    installer->install(packages, system_database.get());

    return 0;
}
