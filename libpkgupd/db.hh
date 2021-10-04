#ifndef _PKGUPD_DATABASE_HH_
#define _PKGUPD_DATABASE_HH_

#include "defines.hh"
#include "pkginfo.hh"

namespace rlxos::libpkgupd {
class db : public object {
   protected:
    std::string _data_dir;

   public:
    db(std::string const &datadir)
        : _data_dir{datadir} {
    }

    GET_METHOD(std::string, data_dir);

    virtual std::shared_ptr<pkginfo> operator[](std::string const &pkgid) = 0;
};
}  // namespace rlxos::libpkgupd

#endif