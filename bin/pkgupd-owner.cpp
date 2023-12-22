#include "common.h"


#include <algorithm>
#include <fstream>


PKGUPD_MODULE_HELP(owner) {
    os << "Search the package who provide specified file." << std::endl;
}

PKGUPD_MODULE(owner) {
    CHECK_ARGS(1);
    engine->load_system_database();
    std::filesystem::path filepath = args[0];
    bool found = false;
    for (auto const &[_, installed_meta_info]: engine->list_installed()) {
        std::vector<std::string> files_list;
        engine->list_installed_files(installed_meta_info, files_list);
        if (auto i = std::find_if(files_list.begin(), files_list.end(),
                                  [&filepath](std::string const &path) -> bool {
                                      if (path.length() < 2) return false;
                                      return filepath.compare("/" + path.substr(2)) == 0;
                                  }); i != files_list.end()) {
            std::cout << BOLD("Provided by") << " " << GREEN(installed_meta_info.id)
                      << std::endl;
            found = true;
        }
    }
    if (!found) {
        ERROR("no package provide " << filepath);
        return 1;
    }
    return 0;
}
