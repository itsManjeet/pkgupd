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

  bool resolve(std::shared_ptr<PackageInfo> const &info,
               std::vector<std::shared_ptr<PackageInfo>> &list);

 public:
  Resolver(GetPackageFunctionType get_fun, SkipPackageFunctionType skip_fun)
      : mGetPackageFunction{get_fun}, mSkipPackageFunction{skip_fun} {}

  bool depends(std::string id, std::vector<std::shared_ptr<PackageInfo>> &list);

  bool depends(std::shared_ptr<PackageInfo> const &info,
               std::vector<std::shared_ptr<PackageInfo>> &list);
};
}  // namespace rlxos::libpkgupd

#endif