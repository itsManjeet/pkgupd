#ifndef LIBPKGUPD_RECIPE
#define LIBPKGUPD_RECIPE

#include <yaml-cpp/yaml.h>

#include <optional>

#include "builder.hh"
#include "defines.hh"
#include "package.hh"

namespace rlxos::libpkgupd {

struct SplitPackage {
  std::string into;
  std::string about;
  std::vector<std::string> depends;

  std::vector<std::string> files;
};

class Recipe {
 private:
  std::string m_ID, m_Version, m_About;
  PackageType m_PackageType;
  std::vector<std::string> m_Depends, m_BuildTime;

  BuildType m_BuildType;

  std::vector<std::string> m_Environ, m_Sources;
  std::string m_BuildDir;
  std::string m_Configure, m_Compile, m_Install;
  std::string m_PreScript, m_PostScript;
  std::vector<SplitPackage> m_SplitPackages;

  std::vector<std::string> m_SkipStrip;
  bool m_DoStrip;

  std::string m_InstallScript;

  std::string m_Script;
  std::string m_Repository;

  std::vector<User> m_Users;
  std::vector<Group> m_Groups;

  YAML::Node m_Node;

 public:
  Recipe(YAML::Node data, std::string file, std::string const& repo);

  std::string const& id() const { return m_ID; }
  std::string const& version() const { return m_Version; }
  std::string const& about() const { return m_About; }

  PackageType const packageType() const { return m_PackageType; }

  BuildType const buildType() const { return m_BuildType; }

  std::vector<std::string> const& depends() const { return m_Depends; }
  std::vector<std::string> const& buildTime() const { return m_BuildTime; }

  std::string const& buildDir() const { return m_BuildDir; }

  std::string const& configure() const { return m_Configure; }
  std::string const& compile() const { return m_Compile; }
  std::string const& install() const { return m_Install; }

  std::string const& prescript() const { return m_PreScript; }
  std::string const& postscript() const { return m_PostScript; }

  std::string const& script() const { return m_Script; }

  std::string const& installScript() const { return m_InstallScript; }

  std::vector<std::string> const& environ() const { return m_Environ; }
  std::vector<std::string> const& sources() const { return m_Sources; }

  std::vector<std::string> const& skipStrip() const { return m_SkipStrip; }

  std::vector<SplitPackage> const& splits() const { return m_SplitPackages; }

  bool dostrip() const { return m_DoStrip; }

  YAML::Node const& node() const { return m_Node; }

  void dump(std::ostream& os, bool as_meta = false);

  bool contains(std::string const& pkgid) {
    for (auto const& i : packages()) {
      if (i.id() == pkgid) {
        return true;
      }
    }
    return false;
  }

  std::optional<Package> operator[](std::string const& name) const;
  std::vector<Package> packages() const;
};
}  // namespace rlxos::libpkgupd

#endif