#include "gem.hh"

#include "../downloader.hh"
#include "../exec.hh"
#include "../recipe.hh"

namespace rlxos::libpkgupd {
bool Gem::compile(Recipe const& recipe, std::string dir, std::string destdir,
                  std::vector<std::string>& environ) {
  std::string gem = "gem";
  if (recipe.node()["GEM"]) {
    gem = recipe.node()["GEM"].as<std::string>();
  }

  std::string GEMDIR;
  {
    auto [status, output] =
        Executor().output(gem + " env gemdir", dir, environ);
    if (status != 0 || output.length() == 0) {
      p_Error = "failed to get gemdir";
      return false;
    }

    GEMDIR = output.substr(0, output.length() - 1);
  }

  std::string pkgid = recipe.id();
  if (pkgid.rfind("ruby-", 0) == 0) {
    pkgid = pkgid.substr(5, pkgid.length() - 5);
  }

  if (recipe.sources().size() == 0) {
    Downloader downloader({}, "");
    if (!downloader.download(
            "https://rubygems.org/downloads/" + pkgid + "-" + recipe.version() +
                ".gem",
            dir + "/" + pkgid + "-" + recipe.version() + ".gem")) {
      p_Error = "failed to get ruby gem";
      return false;
    }
  }

  std::string InstDir =
      destdir + "/" + GEMDIR + "/gems/" + pkgid + "-" + recipe.version();

  int status = Executor().execute(
      gem +
          " install --ignore-dependencies --no-user-install --no-document -i " +
          destdir + "/" + GEMDIR + " -n " + destdir + "/usr/bin " + pkgid +
          "-" + recipe.version() + ".gem",
      dir, environ);
  if (status != 0) {
    p_Error = "failed to install using ruby gem";
    return false;
  }

  return true;
}
}  // namespace rlxos::libpkgupd