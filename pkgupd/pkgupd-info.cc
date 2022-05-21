#include "../libpkgupd/repository.hh"
#include "../libpkgupd/system-database.hh"
using namespace rlxos::libpkgupd;

#include <iostream>
using namespace std;

PKGUPD_MODULE_HELP(info) { os << "print package information" << endl; }

PKGUPD_MODULE(info) {
  auto system_database = std::make_shared<SystemDatabase>(config);
  auto repository = std::make_shared<Repository>(config);

  CHECK_ARGS(1);

  auto package_id = args[0];
  shared_ptr<PackageInfo> package;

  package = system_database->get(package_id.c_str());
  if (package == nullptr) {
    package = repository->get(package_id.c_str());
  }

  if (package == nullptr) {
    cerr << "Error! no package found with id '" + package_id + "'" << endl;
    return 2;
  }

  string info_value = config->get<string>("info.value", "");
  if (info_value.length() == 0) {
    cout << "id         : " << package->id() << '\n'
         << "version    : " << package->version() << '\n'
         << "about      : " << package->about() << '\n'
         << "repository : " << package->repository() << '\n'
         << "type       : "
         << PACKAGE_TYPE_ID[PACKAGE_TYPE_INT(package->type())] << endl;

#define INSTALLED(in) std::static_pointer_cast<InstalledPackageInfo>(in)

    auto installed_info =
        std::static_pointer_cast<InstalledPackageInfo>(package);
    if (installed_info != nullptr) {
      cout << "installed  : " << installed_info->installed_on() << '\n'
           << "files      : " << to_string(installed_info->files().size())
           << endl;
    }
  } else {
    if (info_value == "id") {
      cout << package->id();
    } else if (info_value == "version") {
      cout << package->version();
    } else if (info_value == "about") {
      cout << package->about();
    } else if (info_value == "repository") {
      cout << package->repository();
    } else if (info_value == "type") {
      cout << PACKAGE_TYPE_ID[PACKAGE_TYPE_INT(package->type())];
    } else if (info_value == "installed") {
      cout << (INSTALLED(package) != nullptr ? "true" : "false");
    } else if (info_value == "installed.time") {
      cout << (INSTALLED(package) == nullptr
                   ? ""
                   : INSTALLED(package)->installed_on());
    } else if (info_value == "files") {
      if (INSTALLED(package) != nullptr) {
        for (auto const& i : INSTALLED(package)->files()) {
          cout << i << endl;
        }
      }
    } else if (info_value == "files.count") {
      if (INSTALLED(package) != nullptr) {
        cout << INSTALLED(package)->files().size();
      }
    }

    cout << endl;
  }

#undef INSTALLED
  return 0;
}