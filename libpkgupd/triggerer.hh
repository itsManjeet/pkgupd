#ifndef _PKGUPD_TRIGGERER_HH_
#define _PKGUPD_TRIGGERER_HH_

#include "defines.hh"
#include "package.hh"

namespace rlxos::libpkgupd {
class Triggerer : public Object {
 public:
  enum class type : int {
    INVALID,
    MIME,
    DESKTOP,
    FONTS_SCALE,
    HARDWARE,
    UDEV,
    ICONS,
    GTK3_INPUT_MODULES,
    GTK2_INPUT_MODULES,
    GLIB_SCHEMAS,
    GIO_MODULES,
    GDK_PIXBUF,
    FONTS_CACHE,
  };

 protected:
  std::string _mesg(type t);

  std::string _regex(type t);

  type _get(std::string const &path);

  bool _exec(type t);

  std::vector<type> _get(
      std::vector<std::vector<std::string>> const &fileslist);

 public:
  Triggerer() {}
  bool trigger(std::vector<std::vector<std::string>> const &fileslist);

  bool trigger(std::vector<Package> const &pkgs);

  bool trigger();
};
}  // namespace rlxos::libpkgupd

#endif