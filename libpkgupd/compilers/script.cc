#include "script.hh"

#include "../exec.hh"
#include "../recipe.hh"

namespace rlxos::libpkgupd {
bool Script::compile(Recipe const& recipe, std::string dir, std::string destdir,
                     std::vector<std::string>& environ) {
  if (int status = Executor().execute(recipe.script(), dir, environ);
      status != 0) {
    p_Error = "failed to execute build script";
    return false;
  }
  return true;
}
}  // namespace rlxos::libpkgupd