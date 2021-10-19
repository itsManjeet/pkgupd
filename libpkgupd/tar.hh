#ifndef _PKGUPD_TAR_HH_
#define _PKGUPD_TAR_HH_

#include "archive.hh"

namespace rlxos::libpkgupd {
class tar : public archive {
   public:
    tar(std::string const &p) : archive(p) {}
    std::tuple<int, std::string> getdata(std::string const &filepath);

    std::shared_ptr<archive::package> info();

    std::vector<std::string> list();

    bool is_exist(std::string const &path) {
        auto [status, output] = getdata(path);
        return status == 0;
    }

    bool extract(std::string const &outdir);

    bool compress(std::string const &srcdir, std::shared_ptr<pkginfo> const &info);
};
}  // namespace rlxos::libpkgupd
#endif