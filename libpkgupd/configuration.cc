#include "configuration.hh"

#include "colors.hh"
using namespace rlxos::libpkgupd;

#include <filesystem>

std::shared_ptr<Configuration> Configuration::create() {
  std::string PKGUPD_CONFIG_PATH = "/etc/pkgupd.yml";
  if (getenv("PKGUPD_CONFIG_PATH") != nullptr) {
    PKGUPD_CONFIG_PATH = getenv("PKGUPD_CONFIG_PATH");
  }
  YAML::Node node;
  auto mergeConfig = [&node](std::string const& file) {
    try {
      YAML::Node n = YAML::LoadFile(file);
      for (auto const& i : n) {
        node[i.first] = i.second;
      }
    } catch (std::exception const& exc) {
      ERROR("failed to load " << file << ", " << exc.what());
    }
  };

  if (std::filesystem::exists(PKGUPD_CONFIG_PATH)) {
    mergeConfig(PKGUPD_CONFIG_PATH);
  }

  if (std::filesystem::exists(PKGUPD_CONFIG_PATH + ".d")) {
    for (auto const& i :
         std::filesystem::directory_iterator(PKGUPD_CONFIG_PATH + ".d")) {
      if (i.is_regular_file() && i.path().has_extension() &&
          i.path().extension() == ".yml") {
        mergeConfig(i.path().string());
      }
    }
  }
  return std::make_shared<Configuration>(node);
}