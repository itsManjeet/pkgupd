#include "../libpkgupd/builder.hh"
#include "../libpkgupd/installer/installer.hh"
#include "../libpkgupd/recipe.hh"
#include "../libpkgupd/repository.hh"
#include "../libpkgupd/resolver.hh"
#include "../libpkgupd/source-repository.hh"
#include "../libpkgupd/system-database.hh"

using namespace rlxos::libpkgupd;

#include <iostream>
using namespace std;

PKGUPD_MODULE(install);

PKGUPD_MODULE_HELP(build) {
  os << "Build the specified package either from recipe file or from the "
        "source repository."
     << endl;
}

PKGUPD_MODULE(build) {
  CHECK_ARGS(1);

  std::shared_ptr<SourceRepository> sourceRepository =
      std::make_shared<SourceRepository>(config);

  std::shared_ptr<Repository> repository = std::make_shared<Repository>(config);
  std::shared_ptr<SystemDatabase> system_database =
      std::make_shared<SystemDatabase>(config);

  std::shared_ptr<Installer> installer = std::make_shared<Installer>(config);
  std::shared_ptr<Builder> builder = std::make_shared<Builder>(config);

  std::string recipe_file = args[0];
  std::shared_ptr<Recipe> required_recipe;

  if (filesystem::exists(recipe_file)) {
    auto node = YAML::LoadFile(recipe_file);
    required_recipe = std::make_shared<Recipe>(
        node, recipe_file,
        config->get<std::string>("build.repository", "testing"));
  } else {
    PROCESS("searching required recipe file '" << recipe_file << "'");
    required_recipe = sourceRepository->get(recipe_file.c_str());
  }

  if (required_recipe == nullptr) {
    ERROR("failed to get required recipe file '" << BOLD(recipe_file) << "'");
    return 1;
  }

  if (config->get("build.depends", true)) {
    PROCESS("generating dependency tree");
    std::shared_ptr<Resolver> resolver = std::make_shared<Resolver>(
        DEFAULT_GET_PACKAE_FUNCTION, DEFAULT_SKIP_PACKAGE_FUNCTION);
    std::vector<std::string> packages = required_recipe->depends();
    packages.insert(packages.end(), required_recipe->buildTime().begin(),
                    required_recipe->buildTime().end());

    std::vector<PackageInfo*> packagesList;
    for (auto const& i : packages) {
      if (!resolver->depends(i, packagesList)) {
        ERROR(resolver->error());
        return 1;
      }
    }
    for(auto const& i : packages) {
      if (std::find_if(packagesList.begin(), packagesList.end(), [&](PackageInfo* pkg) -> bool {
        return pkg->id() == i;
      }) == packagesList.end()) {
        auto p = repository->get(i.c_str());
        if (p == nullptr) {
          ERROR(repository->error());
          return 1;
        }
        p->setDependency();
        packagesList.push_back(p);
      }
    }

    PROCESS("installing/building required packages");
    config->node()["installer.depends"] = false;

    config->node()["mode.ask"] = false;
    if (!installer->install(packagesList, repository.get(),
                            system_database.get())) {
      ERROR("failed to install dependent package, " << installer->error());
      return 1;
    }
  }

  PROCESS("build main package");
  if (!builder->build(required_recipe.get(), system_database.get(), repository.get())) {
    cerr << "Error! " << builder->error() << endl;
    return 1;
  }
  return 0;
}