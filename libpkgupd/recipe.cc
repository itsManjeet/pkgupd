#include "recipe.hh"

#include <sstream>

#include "defines.hh"

using std::string;

namespace rlxos::libpkgupd {
Recipe::Recipe(YAML::Node data, std::string file, std::string const& repo)
    : m_Repository(repo), mFilePath(file) {
  m_Node = data;

  READ_VALUE(string, "id", m_ID);
  READ_VALUE(string, "version", m_Version);

  // If release is provided respect it
  if (data["release"]) {
    m_Version += "-" + data["release"].as<string>();
  }

  READ_VALUE(string, "about", m_About);

  READ_LIST_FROM(string, runtime, depends, m_Depends);
  READ_LIST_FROM(string, buildtime, depends, m_BuildTime);

  READ_OBJECT_LIST(User, "users", m_Users);
  READ_OBJECT_LIST(Group, "groups", m_Groups);

  READ_LIST(string, "sources", m_Sources);
  READ_LIST(string, "environ", m_Environ);

  READ_LIST(string, "skip-strip", m_SkipStrip);
  READ_LIST(string, "backup", m_Backup);

  READ_LIST(string, "include", m_Include);

  OPTIONAL_VALUE(bool, "strip", m_DoStrip, true);

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

    OPTIONAL_VALUE(string, "install_script", m_InstallScript, "");

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
      buildType = BUILD_TYPE_NAME[BUILD_TYPE_INT(
          BUILD_TYPE_FROM_FILE(configurator.c_str()))];
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

    OPTIONAL_VALUE(string, "install-script", m_InstallScript, "");

    if (data["split"]) {
      for (auto const& i : data["split"]) {
        SplitPackage splitPackage;

        splitPackage.into = i["into"].as<std::string>();
        splitPackage.about = i["about"].as<std::string>();

        _R(splitPackage.about);
        for (auto f : i["files"]) {
          auto file = f.as<std::string>();
          _R(file);
          splitPackage.files.push_back(file);
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

  m_PackageType = PACKAGE_TYPE_FROM_STR(packageType.c_str());

  if (buildType.length()) {
    m_BuildType = BUILD_TYPE_FROM_STR(buildType.c_str());
  } else {
    m_BuildType = BuildType::N_BUILD_TYPE;
  }

  _R(m_About);
  _RL(m_Environ);
  _RL(m_Sources);
  _R(m_BuildDir);
  _R(m_Configure);
  _R(m_Compile);
  _R(m_Install);
  _R(m_PreScript);
  _R(m_PostScript);
  _RL(m_SkipStrip);
  _R(m_InstallScript);
  _R(m_Script);
}

std::shared_ptr<PackageInfo> Recipe::operator[](std::string const& name) const {
  for (auto i : packages()) {
    if (i->id() == name) {
      return i;
    }
  }
  return nullptr;
}

void Recipe::dump(std::ostream& os, bool as_meta) {
  if (!as_meta) {
    os << as_meta;
    return;
  }
  std::stringstream ss;
  ss << m_Node;

  std::string line;
  std::getline(ss, line, '\n');
  os << "  - " << line << std::endl;
  while (std::getline(ss, line, '\n')) {
    os << "    " << line << std::endl;
  }
}

std::vector<std::shared_ptr<PackageInfo>> Recipe::packages() const {
  std::vector<std::shared_ptr<PackageInfo>> packagesList;
  packagesList.push_back(std::make_shared<PackageInfo>(
      m_ID, m_Version, m_About, m_Depends, m_PackageType, m_Users, m_Groups,
      m_Backup, m_InstallScript, m_Repository, m_Node));

  for (auto const& i : m_SplitPackages) {
    packagesList.push_back(std::make_shared<PackageInfo>(
        i.into, m_Version, i.about.size() ? i.about : m_About,
        i.depends.size() ? i.depends : m_Depends, m_PackageType, m_Users,
        m_Groups, m_Backup, m_InstallScript, m_Repository, m_Node));
  }

  return packagesList;
}
}  // namespace rlxos::libpkgupd