#ifndef LIBPKGUPD_BUILDER
#define LIBPKGUPD_BUILDER

#include "defines.hh"

namespace rlxos::libpkgupd {

enum class BuildType {
  CMAKE,
  MESON,
  SCRIPT,
  AUTOCONF,
};

static std::string buildTypeToString(BuildType type) {
  switch (type) {
    case BuildType::CMAKE:
      return "cmake";
    case BuildType::MESON:
      return "meson";
    case BuildType::SCRIPT:
      return "script";
    case BuildType::AUTOCONF:
      return "autoconf";
    default:
      throw std::runtime_error("unimplemented buildtype");
  }
}
static BuildType stringToBuildType(std::string type) {
  if (type == "cmake") {
    return BuildType::CMAKE;
  } else if (type == "meson") {
    return BuildType::MESON;
  } else if (type == "script") {
    return BuildType::SCRIPT;
  } else if (type == "autoconf") {
    return BuildType::AUTOCONF;
  }
  throw std::runtime_error("unimplemented build type '" + type + "'");
}

class Recipe;

class Builder : public Object {
 private:
  std::string m_BuildDir, m_SourceDir, m_PackageDir;

  bool prepare(std::vector<std::string> const& sources, std::string const& dir);

  bool pack(std::vector<std::string> const& dirs);

 protected:
  std::string const PREFIX = "/usr";
  std::string const SYSCONF_DIR = "/etc";
  std::string const BINDIR = "/usr/bin";
  std::string const SBINDIR = "/usr/bin";
  std::string const LIBDIR = "/usr/lib";
  std::string const LIBEXEDIR = "/usr/lib";
  std::string const DATADIR = "/usr/share";
  std::string const CACHEDIR = "/var";

  virtual bool compile(Recipe const& recipe, std::string dir,
                       std::string destdir,
                       std::vector<std::string> const& environ) = 0;

 public:
  void set(std::string const& builddir, std::string const& sourcedir,
           std::string const& packagedir) {
    m_BuildDir = builddir;
    m_SourceDir = sourcedir;
    m_PackageDir = packagedir;
  }

  bool build(Recipe const& recipe);

  static std::shared_ptr<Builder> create(BuildType buildType);
};
}  // namespace rlxos::libpkgupd

#endif