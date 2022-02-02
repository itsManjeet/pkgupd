#include "recipe.hh"

#include "defines.hh"

using std::string;

namespace rlxos::libpkgupd {
Recipe::Recipe(YAML::Node data, std::string file) {
  m_Node = data;

  READ_VALUE(string, "id", m_ID);
  READ_VALUE(string, "version", m_Version);
  READ_VALUE(string, "about", m_About);

  READ_LIST_FROM(string, runtime, depends, m_Depends);
  READ_LIST_FROM(string, buildtime, depends, m_BuildTime);

  std::string packageType;
  OPTIONAL_VALUE(string, "type", packageType, "rlx");
  m_PackageType = stringToPackageType(packageType);

  READ_OBJECT_LIST(User, "users", m_Users);
  READ_OBJECT_LIST(Group, "groups", m_Groups);

  if (data["build-type"]) {
    std::string buildType;
    READ_VALUE(string, "build-type", buildType);
    m_BuildType = stringToBuildType(buildType);
  } else {
    m_BuildType = BuildType::INVALID;
  }

  READ_LIST(string, "sources", m_Sources);
  READ_LIST(string, "environ", m_Environ);

  READ_LIST(string, "skip-strip", m_SkipStrip);

  OPTIONAL_VALUE(bool, "strip", m_DoStrip, false);

  OPTIONAL_VALUE(string, "configure", m_Configure, "");
  OPTIONAL_VALUE(string, "compile", m_Compile, "");
  OPTIONAL_VALUE(string, "install", m_Install, "");

  OPTIONAL_VALUE(string, "build-dir", m_BuildDir, "");

  OPTIONAL_VALUE(string, "script", m_Script, "");
  OPTIONAL_VALUE(string, "pre-script", m_PreScript, "");
  OPTIONAL_VALUE(string, "post-script", m_PostScript, "");

  if (data["split"]) {
    for (auto const& i : data["split"]) {
      SplitPackage splitPackage;

      splitPackage.into = i["into"].as<std::string>();
      splitPackage.about = i["about"].as<std::string>();

      for (auto const& file : i["files"]) {
        splitPackage.files.push_back(file.as<std::string>());
      }

      if (i["depends"]) {
        for (auto const& dep : i["depends"]) {
          splitPackage.depends.push_back(dep.as<std::string>());
        }
      }
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