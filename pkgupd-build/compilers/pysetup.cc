#include "pysetup.hh"

#include "../exec.hh"
#include "../recipe.hh"

namespace rlxos::libpkgupd {
bool PySetup::compile(Recipe const& recipe, std::string dir,
                      std::string destdir, std::vector<std::string>& environ) {
  if (int status = Executor().execute(
          "python setup.py build " + recipe.compile(), dir, environ);
      status != 0) {
    p_Error = "failed to build with python pysetup.py";
    return false;
  }

  if (int status = Executor().execute(
          "python setup.py install --root='" + destdir + "' --optimize=1",
          dir, environ);
      status != 0) {
    p_Error = "failed to install with python pysetup.py";
    return false;
  }
  return true;
}
}  // namespace rlxos::libpkgupd