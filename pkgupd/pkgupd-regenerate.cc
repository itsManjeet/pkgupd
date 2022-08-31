#include "../libpkgupd/configuration.hh"
#include "../libpkgupd/system-database.hh"
using namespace rlxos::libpkgupd;

#include <fstream>

PKGUPD_MODULE_HELP(regenerate) {
  os << "regenerate pkgupd database for updated datastructure";
}

PKGUPD_MODULE(regenerate) {
  auto system_database = SystemDatabase(config);
  auto database_path = std::filesystem::path(
      config->get<std::string>(DIR_DATA, DEFAULT_DATA_DIR));
  for (auto const& i : system_database.get()) {
    auto package_info = i.second.get();
    auto package_info_file = database_path / package_info->id();
    auto package_info_files_list = package_info_file.string() + ".files";
    if (!std::filesystem::exists(package_info_files_list)) {
      std::ofstream files_writer(package_info_files_list);
      for (auto const& i : package_info->node()["files"]) {
        files_writer << i.as<std::string>() << std::endl;
      }
      files_writer.close();

      auto node = package_info->node();
      node.remove("files");
      std::ofstream info_writer(package_info_file);
      info_writer << node << std::flush;
      info_writer.close();
    }
  }
}