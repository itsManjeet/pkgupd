#ifndef _LIBPKGUPD_COMPILER_HH_
#define _LIBPKGUPD_COMPILER_HH_

#include <tuple>
#include <string>
#include <map>
#include "../libpkgupd/Defines.hh"
#include "../libpkgupd/RecipeFile.hh"

namespace rlxos::libpkgupd
{
    class Compiler : public Object
    {
    private:
        std::shared_ptr<RecipePackageInfo> package;

        enum class Configurator
        {
            INVALID,
            AUTOCONF,
            AUTOGEN,
            PYSETUP,
            MESON,
            CMAKE
        };
        enum class Builder
        {
            INVALID,
            NINJA,
            MAKE,
        };

        std::map<std::string, Configurator> configurators =
            {
                {"configure", Configurator::AUTOCONF},
                {"autogen.sh", Configurator::AUTOGEN},
                {"CMakeLists.txt", Configurator::CMAKE},
                {"meson.build", Configurator::MESON},
                {"setup.py", Configurator::PYSETUP},
            };

        std::map<std::string, Builder> builders =
            {
                {"Makefile", Builder::MAKE},
                {"build.ninja", Builder::NINJA},
            };

        Configurator getConfigurator(std::string const &path);

        Builder getBuilder(std::string const &path);

    public:
        Compiler(std::shared_ptr<RecipePackageInfo> &package)
            : package{package} {}

        bool Compile(std::string const &srcdir, std::string const &pkgdir);
    };
}

#endif