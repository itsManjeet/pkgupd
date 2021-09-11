#ifndef _LIBPKGUPD_REMOVER_HH_
#define _LIBPKGUPD_REMOVER_HH_

#include "Defines.hh"
#include "SystemDatabase.hh"
#include "Triggerer.hh"

namespace rlxos::libpkgupd
{
    class Remover : public Object
    {
    private:
        std::string rootdir;
        std::shared_ptr<SystemDatabase> mSystemDatabase;
        Triggerer mTriggerer;

        std::vector<std::vector<std::string>> fileslist;

        bool isSkipTriggers = false;

    public:
        Remover(std::string const &rootdir)
            : rootdir(rootdir)
        {
        }

        void SetSystemDatabase(std::shared_ptr<SystemDatabase> sysdb)
        {
            mSystemDatabase = sysdb;
        }

        void SkipTriggers(bool b)
        {
            isSkipTriggers = b;
        }

        bool Remove(std::shared_ptr<SystemPackageInfo> pkginfo);

        bool Remove(std::vector<std::string> const &pkgs);
    };
}

#endif