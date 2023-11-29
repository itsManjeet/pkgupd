#ifndef _LIBPKGUPD_RESOLVEDEPENDS_HH_
#define _LIBPKGUPD_RESOLVEDEPENDS_HH_

#include <functional>
#include <optional>
#include "defines.hxx"

#define DEFAULT_GET_PACKAE_FUNCTION \
    [&](const std::string& id) -> std::optional<MetaInfo> { return repository->get(id); }

#define DEFAULT_SKIP_PACKAGE_FUNCTION                              \
    [&](const MetaInfo& meta_info) -> bool {                \
        return system_database->get(meta_info.id) != std::nullopt; \
    }

#define DEFAULT_DEPENDS_FUNCTION \
    [&](const MetaInfo& pkg) -> std::vector<std::string> { return pkg.depends; }

namespace rlxos::libpkgupd {
    template<typename Information>
    class Resolver : public Object {
    public:
        std::vector<std::string> visited{};
        using GetPackageFunctionType = std::function<std::optional<Information>(const std::string& id)>;
        using SkipPackageFunctionType = std::function<bool(Information pkg)>;
        using PackageDependsFunctionType =
        std::function<std::vector<std::string>(Information pkg)>;

    private:
        GetPackageFunctionType mGetPackageFunction;
        SkipPackageFunctionType mSkipPackageFunction;
        PackageDependsFunctionType mPackageDependsFunction;

        bool resolve(Information info, std::vector<Information>& list) {
            if (std::find_if(list.begin(), list.end(), [&](Information pkginfo) -> bool {
                return pkginfo.id == info.id;
            }) != list.end()) {
                return true;
            }
            if (mSkipPackageFunction(info)) return true;
            if (std::find(visited.begin(), visited.end(), info.id) != visited.end()) return true;
            visited.push_back(info.id);
            // TODO: set dependency here
            // info->setDependency();

            DEBUG("CHECKING " << info.id);

            for (auto const& i: mPackageDependsFunction(info)) {
                auto dep_info = mGetPackageFunction(i.c_str());
                if (!dep_info) {
                    p_Error = "Failed to get required package " + i;
                    return false;
                }
                if (!resolve(*dep_info, list)) {
                    p_Error += "\n Trace Required by " + info.id;
                    return false;
                }
            }

            if ((std::find_if(list.begin(), list.end(), [&info](const MetaInfo& id) {
                return info.id == id.id;
            }) == list.end())) {
                list.push_back(info);
            }

            return true;
        }

    public:
        Resolver(GetPackageFunctionType get_fun, SkipPackageFunctionType skip_fun,
                 PackageDependsFunctionType depends_func)
            : mGetPackageFunction{get_fun},
              mSkipPackageFunction{skip_fun},
              mPackageDependsFunction{depends_func} {
        }

        bool depends(std::string id, std::vector<Information>& list) {
            auto packageInfo = mGetPackageFunction(id.c_str());
            if (!packageInfo) {
                p_Error = "required package '" + id + "' is missing";
                return false;
            }
            return depends(*packageInfo, list);
        }

        bool depends(Information info, std::vector<Information>& list) {
            for (auto const& depid: mPackageDependsFunction(info)) {
                auto dep = mGetPackageFunction(depid.c_str());
                if (!dep) {
                    p_Error = "\n Missing required dependency " + depid;
                    return false;
                }
                if (!resolve(*dep, list)) {
                    return false;
                }
            }
            return true;
        }
    };
} // namespace rlxos::libpkgupd

#endif
