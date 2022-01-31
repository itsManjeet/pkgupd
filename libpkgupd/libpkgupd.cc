#include "libpkgupd.hh"
#include "recipe.hh"

#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <yaml-cpp/node/parse.h>

namespace rlxos::libpkgupd {
bool Pkgupd::install(std::vector<std::string> const &packages) {
  bool status =
      m_Installer.install(packages, m_RootDir, m_IsSkipTriggers, m_IsForce);
  p_Error = m_Installer.error();
  return status;
}

bool Pkgupd::build(std::string const &recipefile) {
  auto node = YAML::LoadFile(recipefile);
  auto recipe = Recipe(node, recipefile);

  m_BuildDir = "/tmp/pkgupd-" + recipe.id();

  auto builder = Builder::create(recipe.buildType());
  builder->set(m_BuildDir, m_SourceDir, m_PackageDir);

  bool status = builder->build(recipe);
  p_Error = builder->error();

  return status;
}

bool Pkgupd::remove(std::vector<std::string> const &packages) {
  bool status = m_Remover.remove(packages, m_IsSkipTriggers);
  p_Error = m_Remover.error();
  return status;
}

bool Pkgupd::update(std::vector<std::string> const &packages) {
  bool status =
      m_Installer.install(packages, m_RootDir, m_IsSkipTriggers, true);
  p_Error = m_Installer.error();
  return status;
}

std::vector<UpdateInformation> Pkgupd::outdate() {
  std::vector<UpdateInformation> informations;
  for (auto const &installedInformation : m_SystemDatabase.all()) {
    auto repositoryInformation = m_Repository[installedInformation.id()];
    if (!repositoryInformation) {
      continue;
    }

    if (installedInformation.version() != repositoryInformation->version()) {
      UpdateInformation information;
      information.previous = installedInformation;
      information.updated = *repositoryInformation;
      informations.push_back(information);
    }
  }

  return informations;
}

std::vector<std::string> Pkgupd::depends(std::string const &package) {
  if (!m_Resolver.resolve(package)) {
    p_Error = m_Resolver.error();
    return {};
  }
  return m_Resolver.list();
}

std::vector<Package> Pkgupd::search(std::string query) {
  std::vector<Package> packages;
  for (auto const &package : m_SystemDatabase.all()) {
    if (package.id().find(query) != std::string::npos ||
        package.about().find(query) != std::string::npos) {
      packages.push_back(package);
    }
  }
  for (auto const &package : m_Repository.all()) {
    if (package.id().find(query) != std::string::npos ||
        package.about().find(query) != std::string::npos) {
      packages.push_back(package);
    }
  }

  return packages;
}

bool Pkgupd::sync() {
  if (!m_Downloader.get("recipe", "/tmp/.rcp")) {
    p_Error = m_Downloader.error();
    return false;
  }

  try {
    std::error_code err;

    auto node = YAML::LoadFile("/tmp/.rcp");
    if (std::filesystem::exists(m_Repository.path())) {
      std::filesystem::remove_all(m_Repository.path(), err);
      if (err) {
        p_Error = "Failed to remove old repository data, " + err.message();
        return false;
      }
    }

    std::filesystem::create_directories(m_Repository.path(), err);
    if (err) {
      p_Error = "failed to create repository data, " + err.message();
      return false;
    }

    for (auto const &i : node["recipes"]) {
      if (i["id"]) {
        auto id = i["id"].as<std::string>();
        DEBUG("found " << id);
        std::ofstream file(m_Repository.path() + "/" + id + ".yml");
        if (!file.is_open()) {
          p_Error = "failed to open file in " + m_Repository.path() +
                    " to write recipe file for " + id;
          return false;
        }

        file << i << std::endl;
        file.close();
      }
    }

  } catch (YAML::Exception const &err) {
    p_Error = "corrupt data, " + err.msg;
    return false;
  }

  return true;
}

bool Pkgupd::trigger(std::vector<std::string> const &packages) {
  std::vector<Package> packagesList;
  for (auto const &i : packages) {
    auto package = m_SystemDatabase[i];
    if (!package) {
      p_Error = m_SystemDatabase.error();
      return false;
    }
    packagesList.push_back(*package);
  }

  bool status = m_Triggerer.trigger(packagesList);
  p_Error = m_Triggerer.error();

  return status;
}

std::optional<Package> Pkgupd::info(std::string packageName) {
  if (std::filesystem::exists(packageName) &&
      std::filesystem::path(packageName).extension() == ".yml") {
    try {
      YAML::Node node = YAML::LoadFile(packageName);
      auto recipe = Recipe(node, packageName);
      return recipe.packages()[0];
      
    } catch (std::exception const &err) {
      p_Error = "failed to read recipe file, " + std::string(err.what());
      return {};
    }
  }

  auto package = m_SystemDatabase[packageName];
  if (package.has_value()) {
    return package;
  }

  package = m_Repository[packageName];
  if (package.has_value()) {
    return package;
  }

  p_Error = "no package found with name '" + packageName + "'";
  return {};
}
} // namespace rlxos::libpkgupd