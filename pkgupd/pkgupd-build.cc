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
        auto resolver = std::make_shared<Resolver<std::shared_ptr<Recipe>>>(
                [&](const char *id) -> std::shared_ptr<Recipe> {
                    auto recipe =  sourceRepository->get(id);
                    if (recipe == nullptr) {
                        auto idx = string(id).find_first_of(':');
                        if (idx == string::npos) return nullptr;
                        recipe = sourceRepository->get(string(id).substr(0, idx).c_str());
                    }
                    return recipe;
                },
                [&](std::shared_ptr<Recipe> recipe) -> bool {
                    std::vector<string> toskip;
                    config->get("build.skip", toskip);
                    return std::find(toskip.begin(), toskip.end(), recipe->id()) != toskip.end();
                },
                [&](std::shared_ptr<Recipe> recipe) -> std::vector<std::string> {
                    auto depends = recipe->depends();
                    auto build_depends = recipe->buildTime();
                    depends.insert(depends.end(), build_depends.begin(), build_depends.end());
                    return depends;
                });
        std::vector<std::string> packages = required_recipe->depends();
        packages.insert(packages.end(), required_recipe->buildTime().begin(),
                        required_recipe->buildTime().end());

        std::vector<std::shared_ptr<Recipe>> recipesList;
        for (auto const &i: packages) {
            if (!resolver->depends(i, recipesList)) {
                ERROR(resolver->error());
                return 1;
            }
        }
        for (auto const &i: recipesList) {
            bool to_build = false;
            std::vector<PackageInfo *> toInstall;
            for (auto const &sub: i->packages()) {
                auto subPackage = repository->get(i->id().c_str());
                if (subPackage == nullptr) {
                    to_build = true;
                    break;
                }
                toInstall.push_back(subPackage);
            }
            if (to_build) {
                // build this recipe
                if (!builder->build(i.get(), system_database.get(), repository.get())) {
                    ERROR(builder->error());
                    return 1;
                }
            } else {
                if (!installer->install(toInstall, repository.get(), system_database.get())) {
                    ERROR(installer->error());
                    return 1;
                }
            }
        }
    }

    PROCESS("build main package");
    if (!builder->build(required_recipe.get(), system_database.get(), repository.get())) {
        cerr << "Error! " << builder->error() << endl;
        return 1;
    }

    return 0;
}