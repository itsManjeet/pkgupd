#ifndef _PKGUPD_REPOSITORY_DATABASE_HH_
#define _PKGUPD_REPOSITORY_DATABASE_HH_

#include "Database.hh"

namespace rlxos::libpkgupd
{
    class RepositoryDatabase : public Database
    {
    public:
        RepositoryDatabase()
            : Database(DEFAULT_REPO_DIR)
        {
        }

        RepositoryDatabase(std::string const &repodir)
            : Database(repodir)
        {
        }

        std::shared_ptr<PackageInfo> operator[](std::string const &pkgid);
    };
}

#endif