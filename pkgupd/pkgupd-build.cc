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
  os << "build binary package from source file" << endl;
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
    std::vector<std::string> source_packages;

    std::shared_ptr<Resolver> resolver = std::make_shared<Resolver>(
        [&](char const* id) -> std::shared_ptr<PackageInfo> {
          auto packageInfo = repository->get(id);
          auto recipe = sourceRepository->get(id);

          if (recipe == nullptr) return packageInfo;

          if (packageInfo != nullptr && recipe != nullptr) {
            if (recipe->version() == packageInfo->version()) {
              return packageInfo;
            }
          }

          if (recipe != nullptr) {
            source_packages.push_back(id);
            std::vector<std::string> depends = recipe->depends();
            depends.insert(depends.end(), recipe->buildTime().begin(),
                           recipe->buildTime().end());

            return std::make_shared<PackageInfo>(
                recipe->id(), recipe->version(), recipe->about(), depends,
                recipe->packageType(), recipe->packages()[0]->users(),
                recipe->packages()[0]->groups(),
                recipe->packages()[0]->script(),
                recipe->packages()[0]->repository(), recipe->node());
          }
          return nullptr;
        },
        DEFAULT_SKIP_PACKAGE_FUNCTION);
    std::vector<std::string> packages = required_recipe->depends();
    packages.insert(packages.end(), required_recipe->buildTime().begin(),
                    required_recipe->buildTime().end());

    for (auto const& i : packages) {
      if (!resolver->resolve(i)) {
        ERROR(resolver->error());
        for (auto const& i : resolver->missing()) {
          std::cout << " - " << i << std::endl;
        }
        return 1;
      }
    }

    PROCESS("installing/building required packages");
    config->node()["installer.depends"] = false;
    config->node()["mode.ask"] = false;
    for (auto const& i : resolver->list()) {
      auto iter = std::find_if(source_packages.begin(), source_packages.end(),
                               [&](std::string const& source_package) -> bool {
                                 return i->id() == source_package;
                               });
      if (iter == source_packages.end()) {
        if (!installer->install({i}, repository.get(), system_database.get())) {
          ERROR("failed to install dependent package, '" << i->id() << "', "
                                                         << installer->error());
          return 1;
        }
      } else {
        auto source_package = sourceRepository->get(i->id().c_str());
        if (source_package == nullptr) {
          ERROR("internal error, failed to retrieve source package "
                << i->id());
          return 1;
        }

        if (!builder->build(source_package.get())) {
          ERROR("failed to build required recipe, "
                << source_package->id() << ", " << builder->error());
          return 1;
        }

        std::vector<std::string> built_packages = builder->packages();
        if (!installer->install(built_packages, system_database.get())) {
          ERROR("failed to install build packages, " << installer->error());
          return 1;
        }
      }
    }
  }

  PROCESS("build main package");
  if (!builder->build(required_recipe.get())) {
    cerr << "Error! " << builder->error() << endl;
    return 1;
  }
  return 0;
}