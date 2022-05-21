#include "../libpkgupd/repository.hh"
#include "../libpkgupd/system-database.hh"
using namespace rlxos::libpkgupd;

#include <algorithm>
using namespace std;

PKGUPD_MODULE_HELP(search) { os << "search package from repository" << endl; }

PKGUPD_MODULE(search) {
  CHECK_ARGS(1);

  auto package_id = args[0];
  vector<string> packages_list;
  auto system_database = SystemDatabase(config);
  system_database.list_all(packages_list);

  auto repository = Repository(config);
  repository.list_all(packages_list);

  for_each(packages_list.begin(), packages_list.end(),
           [&](string &item) -> void {
             if (item.find(package_id) != string::npos) {
               auto package = system_database.get(item.c_str());
               cout << package->id() << "\t:\t" << package->about() << endl;
             }
           });
  return 0;
}