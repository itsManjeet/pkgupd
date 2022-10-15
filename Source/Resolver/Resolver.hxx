#ifndef _LIBPKGUPD_RESOLVEDEPENDS_HH_
#define _LIBPKGUPD_RESOLVEDEPENDS_HH_

#include <functional>

#include "defines.hxx"
#include "repository.hxx"
#include "system-database.hxx"

#define DEFAULT_GET_PACKAE_FUNCTION                     \
  [&](char const *id) -> PackageInfo* { \
    return repository->get(id);                         \
  }

#define DEFAULT_SKIP_PACKAGE_FUNCTION                          \
  [&](PackageInfo *pkg) -> bool {                              \
    return system_database->get(pkg->id().c_str()) != nullptr; \
  }

namespace libpkgupd {
class Resolver : public Object {
 public:
  using GetPackageFunctionType =
      std::function<PackageInfo*(char const *id)>;
  using SkipPackageFunctionType = std::function<bool(PackageInfo *pkg)>;

 private:
  GetPackageFunctionType mGetPackageFunction;
  SkipPackageFunctionType mSkipPackageFunction;

  bool resolve(PackageInfo* info,
               std::vector<PackageInfo*> &list);

 public:
  Resolver(GetPackageFunctionType get_fun, SkipPackageFunctionType skip_fun)
      : mGetPackageFunction{get_fun}, mSkipPackageFunction{skip_fun} {}

  bool depends(std::string id, std::vector<PackageInfo*> &list);

  bool depends(PackageInfo* info,
               std::vector<PackageInfo*> &list);
};
}  // namespace libpkgupd

#endif