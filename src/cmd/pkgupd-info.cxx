#include "../ArchiveManager/ArchiveManager.hxx"
#include "../recipe.hxx"
#include "../repository.hxx"
#include "../system-database.hxx"

using namespace rlxos::libpkgupd;

#include <fstream>
#include <iostream>

using namespace std;

PKGUPD_MODULE_HELP(info) {
    os << "Display package information of specified package" << endl
            << PADDING << " " << BOLD("Options:") << endl
            << PADDING << "  - info.value=" << BOLD("<param>")
            << "        # show particular information value" << endl
            << PADDING << "  - info.value=" << BOLD("installed.time")
            << " # extra parameter for installed package" << endl
            << PADDING << "  - info.value=" << BOLD("files.count")
            << "    # extra parameter for installed package" << endl
            << endl;
}

PKGUPD_MODULE(info) {
    auto system_database = std::make_shared<SystemDatabase>(config);
    auto repository = std::make_shared<Repository>(config);

    CHECK_ARGS(1);

    auto package_id = args[0];
    std::optional<MetaInfo> meta_info;

    if (filesystem::exists(package_id) &&
        filesystem::path(package_id).has_extension()) {
        auto ext = filesystem::path(package_id).extension().string().substr(1);
        if (ext == "yml") {
            auto recipe = Recipe(
                YAML::LoadFile(package_id));
            meta_info = recipe;
        } else {
            meta_info = ArchiveManager::info(package_id.c_str());
        }
    } else {
        meta_info = system_database->get(package_id.c_str());
    }

    if (!meta_info) {
        meta_info = repository->get(package_id.c_str());
    }
    if (!meta_info) {
        ERROR("Error! no package found with id '" + package_id + "'");
        return 2;
    }

    std::string output_file = config->get<std::string>("info.dump", "");
    if (output_file.size()) {
        std::ofstream writer(output_file);
        if (!writer.is_open()) {
            ERROR("failed to open dump file for writing");
            return 2;
        }
        writer << meta_info->str();
        return 0;
    }

// //     std::vector<std::string> files;
    // //     string info_value = config->get<string>("info.value", "");
    // //     if (info_value.length() == 0) {
    // //         cout << BLUE("Name") << "         :   " << GREEN(package->id()) << '\n'
    // //                 << BLUE("Version") << "      :   " << BOLD(package->version()) << '\n'
    // //                 << BLUE("About") << "        :   " << BOLD(package->about())
    // //                 << endl;
    // //
    // // #define INSTALLED(in) std::dynamic_pointer_cast<InstalledPackageInfo>(in)
    // //
    // //         auto installed_info = std::dynamic_pointer_cast<InstalledPackageInfo>(package);
    // //
    // //         if (installed_info != nullptr) {
    // //             system_database->get_files(installed_info, files);
    // //             cout << BLUE("Installed") << "    :   "
    // //                     << GREEN(installed_info->installed_on()) << '\n'
    // //                     << BLUE("Files") << "        :   " << BOLD(to_string(files.size()))
    // //                     << endl;
    // //             cout << BLUE("Dependency") << "   :   "
    // //                     << BOLD((installed_info->isDependency() ? "true" : "false"))
    // //                     << std::endl;
    // //         }
    // //     } else {
    // //         if (info_value == "id") {
    // //             cout << package->id();
    // //         } else if (info_value == "version") {
    // //             cout << package->version();
    // //         } else if (info_value == "about") {
    // //             cout << package->about();
    // //         } else if (info_value == "installed") {
    // //             cout << (INSTALLED(package) != nullptr ? "true" : "false");
    // //         } else if (info_value == "installed.time") {
    // //             cout << (INSTALLED(package) == nullptr
    // //                          ? ""
    // //                          : INSTALLED(package)->installed_on());
    // //         } else if (info_value == "files") {
    // //             if (INSTALLED(package) != nullptr) {
    // //                 for (auto const& i: files) {
    // //                     cout << i << endl;
    // //                 }
    // //             }
    // //         } else if (info_value == "files.count") {
    // //             if (INSTALLED(package) != nullptr) {
    // //                 cout << files.size();
    // //             }
    // //         }
    //
    //         cout << endl;
    //     }

#undef INSTALLED
    return 0;
}
