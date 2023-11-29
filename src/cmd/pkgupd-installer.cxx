#include <bits/stdc++.h>

using namespace std;

#include "../Installer/installer.hxx"
#include "../repository.hxx"
#include "../system-database.hxx"
#include "../Trigger/Trigger.hxx"

using namespace rlxos::libpkgupd;

PKGUPD_MODULE_HELP(install) {
    os << "Install package from repository" << endl
            << PADDING << " " << BOLD("Options:") << endl
            << PADDING << "  - installer.depends=" << BOLD("<bool>")
            << "   # Enable or disable dependency management" << endl
            << PADDING << "  - force=" << BOLD("<bool>")
            << "               # Force installation of already installed package"
            << endl
            << PADDING << "  - installer.triggers=" << BOLD("<bool>")
            << "  # Skip triggers during installation, also include user/group "
            "creation"
            << endl
            << PADDING << "  - downloader.force=" << BOLD("<bool>")
            << "    # Force redownload of specified package" << endl
            << endl;;
}

PKGUPD_MODULE(install) {
    std::shared_ptr<Installer> installer;
    std::shared_ptr<SystemDatabase> system_database;
    std::shared_ptr<Repository> repository;

    system_database = std::make_shared<SystemDatabase>(config);
    repository = std::make_shared<Repository>(config);
    installer = std::make_shared<Installer>(config);

    std::vector<MetaInfo> packages;
    for (auto const& i: args) {
        auto package_info = repository->get(i.c_str());
        if (!package_info) {
            ERROR("required package '" << i << "' not found");
            return 1;
        }
        packages.push_back(*package_info);
    }
    installer->install(packages, repository.get(), system_database.get());

    return 0;
}
