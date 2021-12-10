#ifndef _LIBPKGUPD_COMPILER_HH_
#define _LIBPKGUPD_COMPILER_HH_

#include <map>
#include <string>
#include <tuple>

#include "defines.hh"
#include "recipe.hh"

#define RLXOS_PREFIX "/usr"
#define RLXOS_BINDIR RLXOS_PREFIX "/bin"
#define RLXOS_SBINDIR RLXOS_BINDIR
#define RLXOS_LIBDIR RLXOS_PREFIX "/lib"
#define RLXOS_LIBEXECDIR RLXOS_PREFIX "/lib"
#define RLXOS_CACHEDIR "/var"
#define RLXOS_DATADIR RLXOS_PREFIX "/share"
#define RLXOS_SYSCONFDIR "/etc"

namespace rlxos::libpkgupd {
class compiler : public object {
 private:
  std::shared_ptr<recipe::package> _package;

  enum class configurator { INVALID, AUTOCONF, AUTOGEN, PYSETUP, MESON, CMAKE };
  enum class builder {
    INVALID,
    NINJA,
    MAKE,
  };

  std::map<std::string, configurator> _configurators = {
      {"configure", configurator::AUTOCONF},
      {"CMakeLists.txt", configurator::CMAKE},
      {"meson.build", configurator::MESON},
      {"setup.py", configurator::PYSETUP},
  };

  std::map<std::string, builder> _builders = {
      {"Makefile", builder::MAKE},
      {"build.ninja", builder::NINJA},
  };

  configurator _detect_configurator(std::string const &path);

  builder _detect_builder(std::string const &path);

 public:
  compiler(std::shared_ptr<recipe::package> &package) : _package{package} {}

  bool compile(std::string const &srcdir, std::string const &pkgdir);
};
}  // namespace rlxos::libpkgupd

#endif