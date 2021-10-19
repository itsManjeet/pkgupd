#ifndef _PKGUPD_IMAGE_HH_
#define _PKGUPD_IMAGE_HH_

#include "archive.hh"
#include "recipe.hh"

namespace rlxos::libpkgupd {
class image : public archive {
   private:
    std::string _app_run,
        _desktop_file;

    std::string _mimetype(std::string const& path);
    std::set<std::string> _list_lib(std::string const& path);
    std::set<std::string> _list_req(std::string const& appdir);
    std::vector<std::string> _libpath;

   public:
    image(std::string const& p);

    std::tuple<int, std::string> getdata(std::string const& filepath);

    std::shared_ptr<archive::package> info();

    std::vector<std::string> list();

    bool is_exist(std::string const& path) {
        auto [status, output] = getdata(path);
        return status == 0;
    }

    bool extract(std::string const& outdir);

    bool compress(std::string const& srcdir, std::shared_ptr<pkginfo> const& info);
};
}  // namespace rlxos::libpkgupd

#endif