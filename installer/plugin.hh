#ifndef __INSTALLER_PLUGIN__
#define __INSTALLER_PLUGIN__

#include <rlx.hh>
#include "../recipe.hh"
#include "../config.hh"

namespace plugin
{
    using std::string;

    class installer : public rlx::obj
    {
    protected:
        YAML::Node const &_config;

    public:
        installer(YAML::Node const &c)
            : _config(c)
        {
        }

        virtual bool pack(pkgupd::recipe const &, string const &pkgid, string pkgdir, string out) = 0;

        virtual bool unpack(string pkgpath, string root_dir, std::vector<string> &fileslist) = 0;

        virtual std::tuple<pkgupd::recipe *, pkgupd::package *> get(string pkgpath) = 0;
        virtual bool getfile(string pkgpath, string path, string out) = 0;
    };
}

#endif