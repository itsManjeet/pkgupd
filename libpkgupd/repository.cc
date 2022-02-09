#include "repository.hh"

#include "recipe.hh"

namespace rlxos::libpkgupd {
std::optional<Package> Repository::operator[](std::string const &packageName) {
  for (auto const &i : all()) {
    if (i.id() == packageName) {
      return i;
    }
  }

  p_Error = "no recipe file found for " + packageName;
  return {};
}

std::vector<Package> Repository::all() {
  std::vector<Package> packages;
  for (auto const &i : std::filesystem::directory_iterator(p_DataDir)) {
    try {
      if (i.is_directory()) {
        continue;
      }

      auto recipeFilePath = std::filesystem::path(p_DataDir) / i;
      auto node = YAML::LoadFile(recipeFilePath.string());

      auto recipe = Recipe(node, recipeFilePath.string());
      for (auto const &package : recipe.packages()) {
        packages.push_back(package);
      }
    } catch (std::exception const &e) {
      ERROR("failed to read " << e.what() << " skipping");
    }
  }
  return packages;
}
}  // namespace rlxos::libpkgupd