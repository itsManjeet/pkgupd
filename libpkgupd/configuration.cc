#include "configuration.hh"

#include "colors.hh"
using namespace rlxos::libpkgupd;

#include <filesystem>

#define SET_DEFAULT_VALUE(id, value) \
  if (!node[id]) node[id] = (value);

void mergeConfig(YAML::Node& node, std::string const& file) {
  try {
    YAML::Node n = YAML::LoadFile(file);
    for (auto const& i : n) {
      node[i.first] = i.second;
    }
  } catch (std::exception const& exc) {
    ERROR("failed to load " << file << ", " << exc.what());
  }
}

std::shared_ptr<Configuration> Configuration::create(YAML::Node node) {
  SET_DEFAULT_VALUE("debug", false);
  std::string PKGUPD_CONFIG_PATH = "/etc/pkgupd.yml";
  if (getenv("PKGUPD_CONFIG_PATH") != nullptr) {
    PKGUPD_CONFIG_PATH = getenv("PKGUPD_CONFIG_PATH");
  }

  if (std::filesystem::exists(PKGUPD_CONFIG_PATH)) {
    mergeConfig(node, PKGUPD_CONFIG_PATH);
  }

  if (std::filesystem::exists(PKGUPD_CONFIG_PATH + ".d")) {
    for (auto const& i :
         std::filesystem::directory_iterator(PKGUPD_CONFIG_PATH + ".d")) {
      if (i.is_regular_file() && i.path().has_extension() &&
          i.path().extension() == ".yml") {
        mergeConfig(node, i.path().string());
      }
    }
  }

  SET_DEFAULT_VALUE("repos",
                    (std::vector<std::string>{"core", "extra", "apps"}));
  SET_DEFAULT_VALUE(
      "mirrors",
      (std::vector<std::string>{"https://rlxos.dev/storage/stable"}));

  if (node["debug"].as<bool>()) std::cout << "Configuration:\n" << node << std::endl;

  return std::make_shared<Configuration>(node);
}