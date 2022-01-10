#ifndef _PKGUPD_IMAGE_HH_
#define _PKGUPD_IMAGE_HH_

#include "archive.hh"
#include "recipe.hh"

namespace rlxos::libpkgupd {
class Image : public Archive {
 private:
  std::string _mimetype(std::string const& path);
  std::set<std::string> _list_lib(std::string const& path);
  std::set<std::string> _list_req(std::string const& appdir);
  std::vector<std::string> _libpath;

  bool install_icon(std::string const& outdir);

  bool install_desktopfile(std::string const& outdir);

 public:
  Image(std::string const& p);

  std::tuple<int, std::string> getdata(std::string const& filepath);

  std::shared_ptr<Archive::Package> info();

  std::vector<std::string> list();

  bool is_exist(std::string const& path) {
    auto [status, output] = getdata(path);
    return status == 0;
  }

  bool extract(std::string const& outdir);

  bool compress(std::string const& srcdir,
                std::shared_ptr<PackageInformation> const& info);
};
}  // namespace rlxos::libpkgupd

#endif