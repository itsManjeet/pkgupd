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

std::optional<Recipe> Repository::recipe(std::string const &pkgid) {
  auto recipe_path = p_DataDir + "/" + pkgid + ".yml";
  if (std::filesystem::exists(recipe_path)) {
    try {
      auto node = YAML::LoadFile(recipe_path);
      auto recipe_data = Recipe(node, recipe_path);
      return recipe_data;
    } catch (std::exception const &exc) {
      p_Error = "failed to load recipe file " + std::string(exc.what());
      return {};
    }
  }

  for (auto const &i : std::filesystem::directory_iterator(p_DataDir)) {
    if (!(i.is_regular_file() && i.path().has_extension() &&
          i.path().extension() == ".yml")) {
      continue;
    }
    recipe_path = p_DataDir + "/" + i.path().filename().string();
    try {
      YAML::Node node = YAML::LoadFile(recipe_path);
      auto recipe_data = Recipe(node, recipe_path);
      if (recipe_data.contains(pkgid)) {
        return recipe_data;
      }
    } catch (std::exception const &exc) {
      ERROR(exc.what() << " " << recipe_path);
      continue;
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