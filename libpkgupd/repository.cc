#include "repository.hh"

#include "recipe.hh"

namespace rlxos::libpkgupd {
std::optional<Package> Repository::operator[](std::string const &packageName) {
  auto recipeFilePath =
      std::filesystem::path(p_DataDir) / (packageName + ".yml");

  if (std::filesystem::exists(recipeFilePath)) {
    try {
      auto node = YAML::LoadFile(recipeFilePath);
      auto recipe = Recipe(node, recipeFilePath);

      auto package = recipe[packageName];
      if (!package) {
        p_Error = "no package with id '" + packageName +
                 "' found in recipe file " + recipeFilePath.string();
        return {};
      }

      return package;

    } catch (YAML::Exception const &ee) {
      p_Error = "failed to read recipe file '" + recipeFilePath.string() + "' " +
               std::string(ee.what());
      return {};
    }
  }
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