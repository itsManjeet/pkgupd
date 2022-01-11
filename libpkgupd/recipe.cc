#include "recipe.hh"

using std::string;

namespace rlxos::libpkgupd {
Recipe::Recipe(YAML::Node data, std::string file) {
  READ_VALUE(string, id, m_ID);
  READ_VALUE(string, version, m_Version);
  READ_VALUE(string, about, m_About);
  READ_LIST(string, depends, m_Depends);

  std::string packageType;
  READ_VALUE(string, type, packageType);
  m_PackageType = stringToPackageType(packageType);

  OPTIONAL_VALUE(string, "script", m_Script, "");
  READ_OBJECT_LIST(User, "users", m_Users);
  READ_OBJECT_LIST(Group, "groups", m_Groups);

  data = data["build"];

  std::string buildType;
  OPTIONAL_VALUE(string, "type", buildType, "auto");
  m_BuildType = stringToBuildType(buildType);

  READ_LIST(string, "depends", m_BuildTime);
  READ_LIST(string, "sources", m_Sources);
  READ_LIST(string, "environ", m_Environ);

  OPTIONAL_VALUE(string, "configure", m_Configure, "");
  OPTIONAL_VALUE(string, "compile", m_Compile, "");
  OPTIONAL_VALUE(string, "install", m_Install, "");

  if (data["split"]) {
    for (auto const& i : data["split"]) {
      SplitPackage splitPackage;
      READ_VALUE(string, "into", splitPackage.into);
      READ_VALUE(string, "about", splitPackage.about);
      READ_LIST(string, "depends", splitPackage.depends);
      READ_LIST(string, "files", splitPackage.files);

      m_SplitPackages.push_back(splitPackage);
    }
  }
}

std::optional<Package> Recipe::operator[](std::string const& name) const {
  for (auto i : packages()) {
    if (i.id() == name) {
      return i;
    }
  }

  return {};
}

std::vector<Package> Recipe::packages() const {
  std::vector<Package> packagesList;
  packagesList.push_back(Package(m_ID, m_Version, m_About, m_PackageType,
                                 m_Depends, m_Users, m_Groups, m_Script));

  for (auto const& i : m_SplitPackages) {
    std::string id = i.into;
    if (id == "lib") {
      id += m_ID;
    }

    packagesList.push_back(Package(
        id, m_Version, i.about.size() ? i.about : m_About, m_PackageType,
        i.depends.size() ? i.depends : m_Depends, m_Users, m_Groups, m_Script));
  }

  return packagesList;
}
}  // namespace rlxos::libpkgupd