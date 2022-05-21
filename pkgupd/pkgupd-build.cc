#include "../libpkgupd/builder.hh"
#include "../libpkgupd/recipe.hh"
using namespace rlxos::libpkgupd;

#include <iostream>
using namespace std;

PKGUPD_MODULE_HELP(build) {
  os << "build binary package from source file" << endl;
}

PKGUPD_MODULE(build) {
  string recipe_file = config->get<std::string>("build.recipe", "recipe.yml");
  if (!filesystem::exists(recipe_file)) {
    cerr << "Error! no recipe file exists '" << recipe_file << "'" << endl;
    return 1;
  }

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

  std::shared_ptr<Builder> builder = std::make_shared<Builder>(config);
  if (!builder->build(recipe.get())) {
    cerr << "Error! " << builder->error() << endl;
    return 1;
  }
  return 0;
}