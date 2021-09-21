#ifndef _LIBPKGUPD_REMOVER_HH_
#define _LIBPKGUPD_REMOVER_HH_

#include "defines.hh"
#include "sysdb.hh"
#include "triggerer.hh"

namespace rlxos::libpkgupd
{
    class remover : public object
    {
    private:
        std::string _root_dir;
        sysdb _sys_db;
        triggerer _triggerer;

        std::vector<std::vector<std::string>> _files_list;

        bool _skip_trigger = false;

    public:
        remover(sysdb &sdb, std::string const &rootdir)
            : _sys_db{sdb},
              _root_dir{rootdir}
        {
        }

        bool remove(std::shared_ptr<sysdb::package> pkginfo_);

        bool remove(std::vector<std::string> const &pkgs, bool skip_triggers = false);
    };
}

#endif