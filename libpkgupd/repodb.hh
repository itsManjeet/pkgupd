#ifndef _PKGUPD_REPOSITORY_DATABASE_HH_
#define _PKGUPD_REPOSITORY_DATABASE_HH_

#include "db.hh"

namespace rlxos::libpkgupd
{
    class repodb : public db
    {
    public:
        repodb(std::string const &data_dir)
            : db(data_dir)
        {
            DEBUG("Repository Database: " << data_dir);
        }

          std::shared_ptr<pkginfo> operator[](std::string const &pkgid);
    };
}

#endif