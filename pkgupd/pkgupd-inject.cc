#include "../libpkgupd/downloader.hh"
#include "../libpkgupd/installer/installer.hh"
using namespace rlxos::libpkgupd;

#include <iostream>
using namespace std;

PKGUPD_MODULE_HELP(inject) { os << "inject package into system" << std::endl; }
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
    if (!downloader.download(uri.c_str(), filename.c_str())) {
      ERROR("failed to download '" << std::filesystem::path(filename).filename()
                                   << "' '" << downloader.error() << "'");
      return 1;
    }
    uri = filename;
  }

  PROCESS("installing " << std::filesystem::path(uri).filename().string());
  if (!installer->install({uri}, system_database.get())) {
    ERROR(installer->error());
    return 1;
  }

  return 0;
}