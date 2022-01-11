#include "compiler.hh"

#include "exec.hh"

namespace rlxos::libpkgupd {
Compiler::configurator Compiler::_detect_configurator(std::string const &path) {
  std::string touse;

  for (auto const &i : _package->flags()) {
    if (i->id() == "configurator") {
      touse = i->value();
      break;
    }
  }

  if (touse.size()) {
    if (_configurators.find(touse) == _configurators.end())
      return configurator::INVALID;
    else
      return _configurators[touse];
  }

  for (auto const &i : _configurators)
    if (std::filesystem::exists(path + "/" + i.first)) return i.second;

  return configurator::INVALID;
}

Compiler::builder Compiler::_detect_builder(std::string const &path) {
  std::string touse;

  for (auto const &i : _package->flags()) {
    if (i->id() == "builder") {
      touse = i->value();
      break;
    }
  }

  if (touse.size()) {
    if (_builders.find(touse) == _builders.end())
      return builder::INVALID;
    else
      return _builders[touse];
  }

  for (auto const &i : _builders)
    if (std::filesystem::exists(path + "/" + i.first)) return i.second;

  return builder::INVALID;
}

bool Compiler::compile(std::string const &srcdir, std::string const &destdir) {
  std::string builddir = srcdir;

  if (_package->script().size() != 0) {
    if (int status =
            Executor().execute(_package->script(), srcdir, _package->environ());
        status != 0) {
      _error = "script failed with exit code: " + std::to_string(status);
      return false;
    }

    return true;
  }

  auto getargs = [&](std::string const &var, std::string fallback) {
    for (auto const &i : this->_package->flags()) {
      if (i->id() == var) {
        if (i->force()) return i->value();
        return fallback + " " + i->value();
      }
    }
    return fallback;
  };

  auto config = _detect_configurator(srcdir);

  builddir = srcdir + "/pkgupd_build_" + _package->id();
  std::filesystem::create_directories(builddir);

  std::string cmd;

  switch (config) {
    case configurator::AUTOCONF:
      cmd = srcdir + "/configure " +
            getargs("configure",
                    " --prefix=" RLXOS_PREFIX " --sysconfdir=" RLXOS_SYSCONFDIR
                    " --libdir=" RLXOS_LIBDIR " --libexecdir=" RLXOS_LIBEXECDIR
                    " --bindir=" RLXOS_BINDIR " --sbindir=" RLXOS_SBINDIR
                    " --localstatedir=" RLXOS_CACHEDIR
                    " --datadir=" RLXOS_DATADIR);
      break;

    case configurator::MESON:
      cmd = "meson " +
            getargs("configure",
                    " --prefix=" RLXOS_PREFIX " --sysconfdir=" RLXOS_SYSCONFDIR
                    " --libdir=" RLXOS_LIBDIR " --libexecdir=" RLXOS_LIBEXECDIR
                    " --bindir=" RLXOS_BINDIR " --sbindir=" RLXOS_SBINDIR
                    " --localstatedir=" RLXOS_CACHEDIR
                    " --datadir=" RLXOS_DATADIR);
      break;

    case configurator::CMAKE:
      cmd = "cmake -S " + srcdir + " " +
            getargs("configure",
                    "-DCMAKE_INSTALL_PREFIX=" RLXOS_PREFIX
                    " -DCMAKE_INSTALL_SYSCONFDIR=" RLXOS_SYSCONFDIR
                    " -DCMAKE_INSTALL_LIBDIR=" RLXOS_LIBDIR
                    " -DCMAKE_INSTALL_LIBEXECDIR=" RLXOS_LIBEXECDIR
                    " -DCMAKE_INSTALL_BINDIR=" RLXOS_BINDIR
                    " -DCMAKE_INSTALL_SBINDIR=" RLXOS_SBINDIR
                    " -DCMAKE_INSTALL_DATADIR=" RLXOS_DATADIR
                    " -DCMAKE_INSTALL_LOCALSTATEDIR=" RLXOS_CACHEDIR);

      break;

    default:
      _error = "no known configurator found";
      return false;
  }

  if (int status = Executor().execute(cmd, builddir, _package->environ());
      status != 0) {
    _error = "failed to configure, exit code: " + std::to_string(status);
    return false;
  }

  auto builder = _detect_builder(builddir);
  switch (builder) {
    case builder::MAKE:
      cmd = "make " + getargs("compile", "");
      break;
    case builder::NINJA:
      cmd = "ninja " + getargs("compile", "");
      break;

    default:
      _error = "No known configurator found";
      return false;
  }

  if (int status = Executor().execute(cmd, builddir, _package->environ());
      status != 0) {
    _error = "Failed to compile, exit code: " + std::to_string(status);
    return false;
  }

  std::vector<std::string> environ = _package->environ();

  std::string DESTDIR = "DESTDIR=" + destdir;
  if (_package->pack() == "none")
    DESTDIR = "";
  else
    environ.push_back(DESTDIR);

  switch (builder) {
    case builder::MAKE:
      cmd = "make " + getargs("install", DESTDIR + " install");
      break;
    case builder::NINJA:
      cmd = "ninja " + getargs("install", {"install"});
      break;

    default:
      _error = "No known configurator found";
      return false;
  }

  if (int status = Executor().execute(cmd, builddir, environ); status != 0) {
    _error = "failed to compile, exit code: " + std::to_string(status);
    return false;
  }

  return true;
}
}  // namespace rlxos::libpkgupd