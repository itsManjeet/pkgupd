#ifndef __COMPILER_PLUGIN__
#define __COMPILER_PLUGIN__

#include <rlx.hh>
#include "../recipe.hh"
#include "../config.hh"

namespace plugin
{
    using std::string;

    class compiler : public rlx::obj
    {
    protected:
        YAML::Node const &_config;

    public:
        compiler(YAML::Node const &c)
            : _config(c)
        {
        }

        virtual bool compile(pkgupd::recipe *, pkgupd::package *, string, string) = 0;
    };
}

#endif