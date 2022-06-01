#include "../libpkgupd/builder.hh"
#include "../libpkgupd/recipe.hh"
using namespace rlxos::libpkgupd;

#include <iostream>
using namespace std;

PKGUPD_MODULE(install);

PKGUPD_MODULE_HELP(build) {
  os << "build binary package from source file" << endl;
}

PKGUPD_MODULE(build) {
  string recipe_file = config->get<std::string>("build.recipe", "recipe.yml");
  if (!filesystem::exists(recipe_file)) {
    cerr << "Error! no recipe file exists '" << recipe_file << "'" << endl;
    return 1;
  }

  config->node()["build.recipe-dir"] =
      std::filesystem::path(recipe_file).parent_path().string();

  std::shared_ptr<Recipe> recipe;
  try {
    auto node = YAML::LoadFile(recipe_file);
    recipe = std::make_shared<Recipe>(
        node, recipe_file,
        config->get<string>("package.repository", "testing"));
  } catch (std::exception const& e) {
    cerr << "Error! failed to read recipe file '" << recipe_file << ", "
         << e.what() << endl;
    return 1;
  }

  if (config->get("build.depends", true)) {
    std::vector<std::string> packages = recipe->depends();
    packages.insert(packages.end(), recipe->buildTime().begin(),
                    recipe->buildTime().end());
    if (PKGUPD_install(packages, config) != 0) {
      return 1;
    }
  }

  std::shared_ptr<Builder> builder = std::make_shared<Builder>(config);
  if (!builder->build(recipe.get())) {
    cerr << "Error! " << builder->error() << endl;
    return 1;
  }
  return 0;
}