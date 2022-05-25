#ifndef _LIBPKGUPD_RESOLVEDEPENDS_HH_
#define _LIBPKGUPD_RESOLVEDEPENDS_HH_

#include <functional>

#include "defines.hh"
#include "repository.hh"
#include "system-database.hh"

#define DEFAULT_GET_PACKAE_FUNCTION                     \
  [&](char const *id) -> std::shared_ptr<PackageInfo> { \
    return repository->get(id);                         \
  }

#define DEFAULT_SKIP_PACKAGE_FUNCTION                          \
  [&](PackageInfo *pkg) -> bool {                              \
    return system_database->get(pkg->id().c_str()) != nullptr; \
  }

namespace rlxos::libpkgupd {
class Resolver : public Object {
 public:
  using GetPackageFunctionType =
      std::function<std::shared_ptr<PackageInfo>(char const *id)>;
  using SkipPackageFunctionType = std::function<bool(PackageInfo *pkg)>;

 private:
  GetPackageFunctionType mGetPackageFunction;
  SkipPackageFunctionType mSkipPackageFunction;

  std::vector<std::string> mVisited;
  std::vector<std::shared_ptr<PackageInfo>> mPackageList;

  bool toSkip(PackageInfo *info);

 public:
  Resolver(GetPackageFunctionType get_fun, SkipPackageFunctionType skip_fun)
      : mGetPackageFunction{get_fun}, mSkipPackageFunction{skip_fun} {}

  bool resolve(std::shared_ptr<PackageInfo> info);

  void clear();

  std::vector<std::shared_ptr<PackageInfo>> const &list() const {
    return mPackageList;
  }
};
}  // namespace rlxos::libpkgupd

#endif