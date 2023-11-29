#include "../Builder/Builder.hxx"
#include "../Installer/installer.hxx"
#include "../recipe.hxx"
#include "../repository.hxx"
#include "../resolver.hxx"
#include "../source-repository.hxx"
#include "../system-database.hxx"

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
    std::optional<Recipe> required_recipe;

    if (filesystem::exists(recipe_file)) {
        auto node = YAML::LoadFile(recipe_file);
        required_recipe = Recipe(node);
    } else {
        PROCESS("searching required recipe file '" << recipe_file << "'");
        required_recipe = sourceRepository->get(recipe_file.c_str());
    }

    if (!required_recipe) {
        ERROR("failed to get required recipe file '" << BOLD(recipe_file) << "'");
        return 1;
    }

    if (config->get("build.depends", true)) {
        PROCESS("generating dependency tree");
        auto resolver = std::make_shared<Resolver<Recipe>>(
            [&](const std::string& id) -> std::optional<Recipe> {
                auto recipe = sourceRepository->get(id);
                if (!recipe) {
                    auto idx = string(id).find_first_of(':');
                    if (idx == string::npos) return nullopt;
                    recipe = sourceRepository->get(string(id).substr(0, idx).c_str());
                }
                return recipe;
            },
            [&](const Recipe& recipe) -> bool {
                std::vector<string> toskip;
                config->get("build.skip", toskip);
                return std::find(toskip.begin(), toskip.end(), recipe.id) != toskip.end();
            },
            [&](const Recipe& recipe) -> std::vector<std::string> {
                auto depends = recipe.depends;
                auto build_depends = recipe.build_depends;
                depends.insert(depends.end(), build_depends.begin(), build_depends.end());
                return depends;
            });
        std::vector<std::string> packages = required_recipe->depends;
        packages.insert(packages.end(), required_recipe->build_depends.begin(),
                        required_recipe->build_depends.end());

        std::vector<Recipe> recipesList;
        for (auto const& i: packages) {
            if (!resolver->depends(i, recipesList)) {
                ERROR(resolver->error());
                return 1;
            }
        }
        for (auto const& i: recipesList) {
            bool to_build = false;
            std::vector<MetaInfo> toInstall;
            if (repository->get(i.id)) {
                to_build = true;
                toInstall.push_back(i);
            }
            if (to_build) {
                builder->build(i, system_database.get(), repository.get());
            } else {
                installer->install(toInstall, repository.get(), system_database.get());
            }
        }
    }

    PROCESS("build main package");
    builder->build(*required_recipe, system_database.get(), repository.get());
    return 0;
}
