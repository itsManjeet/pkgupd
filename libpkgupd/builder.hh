#ifndef _LIBPKGUPD_BUILDER_HH_
#define _LIBPKGUPD_BUILDER_HH_

#include "recipe.hh"

namespace rlxos::libpkgupd {
class builder : public object {
   private:
    std::vector<std::shared_ptr<recipe::package>> _packages;
    std::vector<std::string> _archive_list;

    std::string _work_dir,
        _pkgs_dir,
        _src_dir;

    bool _force = false;

    bool _prepare(std::vector<std::string> const &sources, std::string const &srcdir);

    bool _compile(std::string const &srcdir, std::string const &pkgdir, std::shared_ptr<recipe::package> package);

    bool _build(std::shared_ptr<recipe::package> package);

   public:
    builder(std::string const &wdir,
            std::string const &pdir,
            std::string const &sdir,
            bool force)
        : _work_dir{wdir},
          _pkgs_dir{pdir},
          _src_dir{sdir},
          _force{force} {}

    ~builder() {
        std::filesystem::remove_all(_work_dir);
    }

    GET_METHOD(std::vector<std::string>, archive_list);

    bool build(std::shared_ptr<recipe> const &recipe_) {
        _work_dir += "/" + recipe_->id();

        for (auto const &dir : {_pkgs_dir, _src_dir, _work_dir}) {
            if (!std::filesystem::exists(dir)) {
                std::error_code err;
                std::filesystem::create_directories(dir, err);
                if (err) {
                    _error = err.message();
                    return false;
                }
            }
        }
        for (auto const &pkg : recipe_->packages())
            if (!_build(pkg)) {
                std::filesystem::remove_all(_work_dir);
                return false;
            }

        return true;
    }
};
}  // namespace rlxos::libpkgupd

#endif