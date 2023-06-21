#include "../repository.hh"
#include "../resolver.hh"
#include "../source-repository.hh"
#include "../system-database.hh"
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
            Resolver<std::shared_ptr<Recipe>>(
                [&](const char* id) -> std::shared_ptr<Recipe> {
                    string package_id = id;
                    auto idx = package_id.find_first_of(':');
                    if (idx != std::string::npos) {
                        package_id = package_id.substr(0, idx);
                    }
                    auto recipe = source_repository->get(package_id.c_str());
                    return recipe;
                },
                [&](std::shared_ptr<Recipe> recipe) -> bool {
                    if (list_all) return false;
                    return true;
                },
                [&](std::shared_ptr<Recipe> recipe) -> std::vector<std::string> {
                    auto depends = recipe->depends();
                    depends.insert(depends.end(), recipe->buildTime().begin(), recipe->buildTime().end());
                    return depends;
                });
        std::vector<std::shared_ptr<Recipe>> recipes;
        for (auto const& i : args) {
            if (!resolver.depends(i, recipes)) {
                ERROR(resolver.error());
                return 1;
            }
        }

        for (auto const& i : args) {
            if (std::find_if(recipes.begin(), recipes.end(),
                             [&](std::shared_ptr<Recipe> const& r) -> bool {
                                 return r->id() == i;
                             }) == recipes.end()) {
                recipes.push_back(source_repository->get(i.c_str()));
            }
        }
        for (auto const& i : recipes) {
            cout << i->id() << endl;
        }
    } else {
        auto resolver =
            Resolver<std::shared_ptr<PackageInfo>>(DEFAULT_GET_PACKAE_FUNCTION, DEFAULT_SKIP_PACKAGE_FUNCTION,
                                                   DEFAULT_DEPENDS_FUNCTION);

        std::vector<std::shared_ptr<PackageInfo>> packagesList;
        for (auto const& i : args) {
            if (!resolver.depends(i, packagesList)) {
                ERROR(resolver.error());
                return 1;
            }
        }

        for (auto const& i : args) {
            if (std::find_if(packagesList.begin(), packagesList.end(),
                             [&](std::shared_ptr<PackageInfo> const& pkginfo) -> bool {
                                 return pkginfo->id() == i;
                             }) == packagesList.end()) {
                packagesList.push_back(repository->get(i.c_str()));
            }
        }
        for (auto const& i : packagesList) {
            cout << i->id() << endl;
        }
    }

    return 0;
}