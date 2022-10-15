#ifndef _LIBPKGUPD_RESOLVEDEPENDS_HH_
#define _LIBPKGUPD_RESOLVEDEPENDS_HH_

#include <functional>

#include "defines.hh"
#include "repository.hh"
#include "system-database.hh"

#define DEFAULT_GET_PACKAE_FUNCTION \
  [&](char const *id) -> PackageInfo * { return repository->get(id); }

#define DEFAULT_SKIP_PACKAGE_FUNCTION                          \
  [&](PackageInfo *pkg) -> bool {                              \
    return system_database->get(pkg->id().c_str()) != nullptr; \
  }

#define DEFAULT_DEPENDS_FUNCTION \
  [&](PackageInfo *pkg) -> std::vector<std::string> { return pkg->depends(); }

namespace rlxos::libpkgupd {
class Resolver : public Object {
 public:
  using GetPackageFunctionType = std::function<PackageInfo *(char const *id)>;
  using SkipPackageFunctionType = std::function<bool(PackageInfo *pkg)>;
  using PackageDependsFunctionType =
      std::function<std::vector<std::string>(PackageInfo *pkg)>;

 private:
  GetPackageFunctionType mGetPackageFunction;
  SkipPackageFunctionType mSkipPackageFunction;
  PackageDependsFunctionType mPackageDependsFunction;

  bool resolve(PackageInfo *info, std::vector<PackageInfo *> &list);

 public:
  Resolver(GetPackageFunctionType get_fun, SkipPackageFunctionType skip_fun,
           PackageDependsFunctionType depends_func)
      : mGetPackageFunction{get_fun},
        mSkipPackageFunction{skip_fun},
        mPackageDependsFunction{depends_func} {}

  bool depends(std::string id, std::vector<PackageInfo *> &list);

  bool depends(PackageInfo *info, std::vector<PackageInfo *> &list);
};
}  // namespace rlxos::libpkgupd

#endif