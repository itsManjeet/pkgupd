#include "../configuration.hh"
#include "../system-database.hh"
#include "../utils/utils.hh"
using namespace rlxos::libpkgupd;

#include <algorithm>
#include <fstream>
using namespace std;

PKGUPD_MODULE_HELP(owner) {
    os << "Search the package who provide specified file." << std::endl;
}

PKGUPD_MODULE(owner) {
    CHECK_ARGS(1);
    std::filesystem::path filepath = args[0];

    std::shared_ptr<SystemDatabase> system_database =
        std::make_shared<SystemDatabase>(config);

    int status = 1;

    for (auto const& p : system_database->get()) {
        auto package_info = p.second;
        std::vector<std::string> files;
        system_database->get_files(package_info, files);
        auto iter =
            std::find_if(files.begin(), files.end(),
                         [&filepath](std::string const& path) -> bool {
                             if (path.length() < 2) return false;
                             return filepath.compare("/" + path.substr(2)) == 0;
                         });
        if (iter != files.end()) {
            std::cout << BOLD("Provided by") << " " << GREEN(package_info->id())
                      << std::endl;
            status = 0;
        }
    }

    return status;
}