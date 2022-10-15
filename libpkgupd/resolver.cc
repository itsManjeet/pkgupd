#include "resolver.hh"
namespace rlxos::libpkgupd {

bool Resolver::depends(std::string id, std::vector<PackageInfo *> &list) {
  auto packageInfo = mGetPackageFunction(id.c_str());
  if (packageInfo == nullptr) {
    p_Error = "required package '" + id + "' is missing";
    return false;
  }
  return depends(packageInfo, list);
}

bool Resolver::depends(PackageInfo *info, std::vector<PackageInfo *> &list) {
  for (auto const &depid : mPackageDependsFunction(info)) {
    auto dep = mGetPackageFunction(depid.c_str());
    if (dep == nullptr) {
      p_Error = "\n Missing required dependency " + depid;
      return false;
    }
    if (!resolve(dep, list)) {
      return false;
    }
  }
  return true;
}

bool Resolver::resolve(PackageInfo *info, std::vector<PackageInfo *> &list) {
  if (std::find_if(list.begin(), list.end(), [&](PackageInfo *pkginfo) -> bool {
        return pkginfo->id() == info->id();
      }) != list.end()) {
    return true;
  }
  if (mSkipPackageFunction(info)) return true;
  info->setDependency();

  for (auto const &i : mPackageDependsFunction(info)) {
    auto dep_info = mGetPackageFunction(i.c_str());
    if (dep_info == nullptr) {
      p_Error = "Failed to get required package " + i;
      return false;
    }
    if (!resolve(dep_info, list)) {
      p_Error += "\n Trace Required by " + info->id();
      return false;
    }
  }

  if ((std::find(list.begin(), list.end(), info) == list.end())) {
    list.push_back(info);
  }

  return true;
}
}  // namespace rlxos::libpkgupd