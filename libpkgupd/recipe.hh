#ifndef LIBPKGUPD_RECIPE
#define LIBPKGUPD_RECIPE

#include <yaml-cpp/yaml.h>

#include <optional>

#include "defines.hh"
#include "package.hh"

namespace rlxos::libpkgupd {

enum class BuildType {
  AUTO_DETECT,
  CMAKE,
};

std::string buildTypeToString(BuildType type);
BuildType stringToBuildType(std::string type);

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
  std::string m_Dir;
  std::string m_Configure, m_Compile, m_Install;
  std::vector<SplitPackage> m_SplitPackages;

  std::string m_Script;

  std::vector<User> m_Users;
  std::vector<Group> m_Groups;

 public:
  Recipe(YAML::Node data, std::string file);

  std::optional<Package> operator[](std::string const& name) const;
  std::vector<Package> packages() const;
};
}  // namespace rlxos::libpkgupd

#endif