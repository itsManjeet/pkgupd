#include "../repository.hxx"
#include "../resolver.hxx"
#include "../source-repository.hxx"
#include "../system-database.hxx"

using namespace rlxos::libpkgupd;

#include <iostream>

using namespace std;

PKGUPD_MODULE_HELP(depends) {
    os << "List all the dependent packages required" << endl
            << PADDING << " " << BOLD("Options:") << endl
            << PADDING << "  - depends.all=" << BOLD("<bool>")
            << "    # List all dependent packages including already installed packages"
            << endl
            << endl;
}

PKGUPD_MODULE(depends) {
    auto repository = std::make_shared<Repository>(config);
    auto system_database = std::make_shared<SystemDatabase>(config);
    auto source_repository = std::make_shared<SourceRepository>(config);

    bool list_all = config->get("depends.all", false);

    if (config->get("depends.sources", false)) {
        auto resolver =
                Resolver<Recipe>(
                    [&](const std::string& id) -> std::optional<Recipe> {
                        string package_id = id;
                        auto idx = package_id.find_first_of(':');
                        if (idx != std::string::npos) {
                            package_id = package_id.substr(0, idx);
                        }
                        auto recipe = source_repository->get(package_id.c_str());
                        return recipe;
                    },
                    [&](const Recipe& recipe) -> bool {
                        if (list_all) return false;
                        return true;
                    },
                    [&](const Recipe& recipe) -> std::vector<std::string> {
                        auto depends = recipe.depends;
                        depends.insert(depends.end(), recipe.build_depends.begin(), recipe.build_depends.end());
                        return depends;
                    });
        std::vector<Recipe> recipes;
        for (auto const& i: args) {
            if (!resolver.depends(i, recipes)) {
                ERROR(resolver.error());
                return 1;
            }
        }

        for (auto const& i: args) {
            if (std::find_if(recipes.begin(), recipes.end(),
                             [&](Recipe const& r) -> bool {
                                 return r.id == i;
                             }) == recipes.end()) {
                recipes.push_back(*source_repository->get(i.c_str()));
            }
        }
        for (auto const& i: recipes) {
            cout << i.id << endl;
        }
    } else {
        auto resolver =
                Resolver<MetaInfo>(DEFAULT_GET_PACKAE_FUNCTION, DEFAULT_SKIP_PACKAGE_FUNCTION,
                                   DEFAULT_DEPENDS_FUNCTION);

        std::vector<MetaInfo> packagesList;
        for (auto const& i: args) {
            if (!resolver.depends(i, packagesList)) {
                ERROR(resolver.error());
                return 1;
            }
        }

        for (auto const& i: args) {
            if (std::find_if(packagesList.begin(), packagesList.end(),
                             [&](MetaInfo const& pkginfo) -> bool {
                                 return pkginfo.id == i;
                             }) == packagesList.end()) {
                packagesList.push_back(*repository->get(i));
            }
        }
        for (auto const& i: packagesList) {
            cout << i.id << endl;
        }
    }

    return 0;
}
