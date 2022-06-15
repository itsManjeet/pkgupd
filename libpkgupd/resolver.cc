#include "resolver.hh"
namespace rlxos::libpkgupd {
void Resolver::clear() {
  mPackageList.clear();
  mVisited.clear();
}

bool Resolver::toSkip(PackageInfo *info) {
  if (mToBreak) return true;
  
  auto iter = std::find_if(mPackageList.begin(), mPackageList.end(),
                           [&](std::shared_ptr<PackageInfo> const &p) -> bool {
                             return (p->id() == info->id());
                           });
  if (iter != mPackageList.end()) return true;
  if (mSkipPackageFunction(info)) return true;

  if ((std::find(mVisited.begin(), mVisited.end(), info->id()) !=
       mVisited.end()))
    return true;

  mVisited.push_back(info->id());
  return false;
}

bool Resolver::resolve(std::string id) {
  auto packageInfo = mGetPackageFunction(id.c_str());
  if (packageInfo == nullptr) {
    p_Error = "required package '" + id + "' is missing";
    mMissing.push_back(id);
    return false;
  }
  return resolve(packageInfo);
}

bool Resolver::resolve(std::shared_ptr<PackageInfo> info) {
  if (mBreakOn.length() && info->id() == mBreakOn) mToBreak = true;

  if (mToBreak == true) return true;
  if (toSkip(info.get())) return true;

  auto depends = info->depends();
  for (auto const &i : depends) {
    auto dep_info = mGetPackageFunction(i.c_str());
    if (dep_info == nullptr) {
      mMissing.push_back(i);
      p_Error = "Failed to get required package " + i;
      return false;
    }
    if (!resolve(dep_info)) {
      p_Error += "\n Trace Required by " + info->id();
      return false;
    }
  }

  if ((std::find(mPackageList.begin(), mPackageList.end(), info) ==
       mPackageList.end()))
    mPackageList.push_back(info);

  return true;
}
}  // namespace rlxos::libpkgupd