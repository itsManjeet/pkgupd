#include "../libpkgupd/repository.hh"
#include "../libpkgupd/system-database.hh"
using namespace rlxos::libpkgupd;

#include <algorithm>
#include <iostream>
using namespace std;

PKGUPD_MODULE_HELP(search) { os << "Search package from repository" << endl; }

PKGUPD_MODULE(search) {
  CHECK_ARGS(1);

  auto package_id = args[0];
  vector<string> packages_list;
  auto repository = Repository(config);
  repository.list_all(packages_list);

  vector<shared_ptr<PackageInfo>> packages;
  int found = 0;
  for_each(
      packages_list.begin(), packages_list.end(), [&](string& item) -> void {
        if (item.find(package_id) != string::npos) {
          std::shared_ptr<PackageInfo> package = repository.get(item.c_str());
          found++;

          cout << GREEN(package->id()) << ": " << BLUE(package->version())
               << "\n  " << BOLD(package->about()) << '\n'
               << endl;
        }
      });
  packages_list.clear();
  if (found == 0) {
    ERROR("no package found with name '" << package_id << "'");
  }

  return 0;
}