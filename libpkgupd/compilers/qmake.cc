#include "qmake.hh"

#include "../exec.hh"
#include "../recipe.hh"

namespace rlxos::libpkgupd {
bool QMake::compile(Recipe const& recipe, std::string dir, std::string destdir,
                    std::vector<std::string>& environ) {
  int status = Executor().execute(
      "qmake PREFIX=/usr QT_INSTALL_PREFIX=/usr LIBDIR=/usr/lib "
      "CONFIG+=no_qt_rpath " +
          recipe.configure(),
      dir, environ);
  if (status != 0) {
    p_Error = "failed to configure with qmake";
    return false;
  }

  status = Executor().execute("make " + recipe.compile(), dir, environ);
  if (status != 0) {
    p_Error = "failed to compile with qmake";
    return false;
  }

  status = Executor().execute("make STRIP=true PREFIX=/usr DESTDIR=" + destdir +
                                  " install " + recipe.install(),
                              dir, environ);
  if (status != 0) {
    p_Error = "failed to install with qmake";
    return false;
  }

  return true;
}
}  // namespace rlxos::libpkgupd