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

  READ_OBJECT_LIST(User, "users", m_Users);
  READ_OBJECT_LIST(Group, "groups", m_Groups);

  READ_LIST(string, "sources", m_Sources);
  READ_LIST(string, "environ", m_Environ);

  READ_LIST(string, "skip-strip", m_SkipStrip);

  OPTIONAL_VALUE(bool, "strip", m_DoStrip, false);

  std::string packageType;
  std::string buildType;

  if (data["packages"]) {
    /**
     * Old recipe file format
     */
    DEBUG("using old recipe file");

    if (data["packages"].size() != 1) {
      throw std::runtime_error(
          "multiple packages in old recipe format is not supported in file '" +
          file + "', got " + std::to_string(data["packages"].size()));
    }

    data = data["packages"][0];

    if (data["depends"]) {
      if (data["depends"]["runtime"]) {
        for (auto const& dep : data["depends"]["runtime"]) {
          m_Depends.push_back(dep.as<string>());
        }
      }
      if (data["depends"]["buildtime"]) {
        for (auto const& dep : data["depends"]["buildtime"]) {
          m_BuildTime.push_back(dep.as<string>());
        }
      }
    }

    if (data["environ"]) {
      for (auto const& i : data["environ"]) {
        m_Environ.push_back(i.as<string>());
      }
    }

    if (data["sources"]) {
      for (auto const& i : data["sources"]) {
        m_Sources.push_back(i.as<string>());
      }
    }

    OPTIONAL_VALUE(string, "pack", packageType, "rlx");
    OPTIONAL_VALUE(string, "script", m_Script, "");
    OPTIONAL_VALUE(string, "prescript", m_PreScript, "");
    OPTIONAL_VALUE(string, "postscript", m_PostScript, "");

    OPTIONAL_VALUE(string, "dir", m_BuildDir, "");

    auto getFlag = [&](string flag_id) -> string {
      if (data["flags"]) {
        for (auto const& i : data["flags"]) {
          if (i["id"].as<string>() == flag_id) {
            return i["value"].as<string>();
          }
        }
      }

      return "";
    };

    m_Configure = getFlag("configure");
    m_Compile = getFlag("compile");
    m_Install = getFlag("install");

    auto configurator = getFlag("configurator");
    if (configurator.length()) {
      buildType = buildTypeToString(buildTypeFromFile(configurator));
      DEBUG("got build type '" + buildType + "'")
    }

  } else {
    /**
     * New recipe file format
     */
    OPTIONAL_VALUE(string, "type", packageType, "pkg");

    OPTIONAL_VALUE(string, "build-type", buildType, "");

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

  m_PackageType = stringToPackageType(packageType);

  if (buildType.length()) {
    m_BuildType = stringToBuildType(buildType);
  } else {
    m_BuildType = BuildType::INVALID;
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