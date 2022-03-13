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

std::vector<Recipe> Repository::recipes(std::string const &repo) {
  std::string repoPath = p_DataDir + "/" + repo;
  if (!std::filesystem::exists(repoPath)) {
    p_Error = repoPath + " repository not exists";
    return {};
  }

  std::vector<Recipe> recipes;
  for (auto const &i : std::filesystem::directory_iterator(repoPath)) {
    std::string recipePath = repoPath + "/" + i.path().filename().string();
    try {
      auto node = YAML::LoadFile(recipePath);
      auto recipe_data = Recipe(node, recipePath, repo);
      recipes.push_back(recipe_data);

    } catch (std::exception const &exc) {
      ERROR("failed to load " << recipePath);
    }
  }

  return recipes;
}

std::optional<Recipe> Repository::recipe(std::string const &pkgid) {
  for (auto const &repo : mRepos) {
    for (auto const &rec : recipes(repo)) {
      if (rec.id() == pkgid) {
        return rec;
      }

      for (auto const &pkg : rec.packages()) {
        if (pkg.id() == pkgid) {
          return rec;
        }
      }
    }
  }
  p_Error = "no recipe file found for " + pkgid;
  return {};
}

std::vector<Package> Repository::all() {
  std::vector<Package> packages;
  for (auto const &repo : mRepos) {
    for (auto const &rec : recipes(repo)) {
      auto pkgs = rec.packages();
      packages.insert(packages.end(), pkgs.begin(), pkgs.end());
    }
  }
  return packages;
}
}  // namespace rlxos::libpkgupd