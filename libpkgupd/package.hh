#ifndef _LIBPKGUPD_PACKAGEINFO_HH_
#define _LIBPKGUPD_PACKAGEINFO_HH_

#include <grp.h>
#include <yaml-cpp/yaml.h>

#include <ostream>

#include "defines.hh"
#include "exec.hh"
#include "group.hh"
#include "user.hh"

namespace rlxos::libpkgupd {
/**
 * @brief Package type helps to determine the type of packaging and
 * installation methods use for that package
 *
 */
enum class PackageType : int {
  APPIMAGE,
  PACKAGE,
  RLXOS,
};

static std::string packageTypeToString(PackageType type) {
  switch (type) {
    case PackageType::APPIMAGE:
      return "app";
    case PackageType::PACKAGE:
      return "pkg";
    case PackageType::RLXOS:
      return "rlx";
    default:
      throw std::runtime_error("invalid package type");
  }
}

static PackageType stringToPackageType(std::string const& type) {
  if (type == "app") {
    return PackageType::APPIMAGE;
  } else if (type == "pkg") {
    return PackageType::PACKAGE;
  } else if (type == "rlx") {
    return PackageType::RLXOS;
  }

  throw std::runtime_error("invalid package type " + type);
}

/**
 * @brief Package holds all the information of rlxos packages
 * their dependencies and required user groups
 */
class Package {
 private:
  std::string m_ID;
  std::string m_Version;
  std::string m_About;

  std::vector<std::string> m_Depends;

  PackageType m_PackageType;

  std::vector<User> m_Users;
  std::vector<Group> m_Groups;

  std::string m_Script;
  std::string m_Repository;

  YAML::Node m_Node;

 public:
  Package() {}

  Package(std::string const& id, std::string const& version,
          std::string const& about, PackageType packageType,
          std::vector<std::string> const& depends,
          std::vector<User> const& users, std::vector<Group> const& groups,
          std::string const& repo, std::string const& script)
      : m_ID(id),
        m_Version(version),
        m_About(about),
        m_PackageType(packageType),
        m_Depends(depends),
        m_Users(users),
        m_Groups(groups),
        m_Script(script),
        m_Repository(repo) {}

  Package(YAML::Node const& data, std::string const& file) {
    READ_VALUE(std::string, "id", m_ID);
    READ_VALUE(std::string, "version", m_Version);
    READ_VALUE(std::string, "about", m_About);
    READ_LIST(std::string, "depends", m_Depends);
    OPTIONAL_VALUE(std::string, "repository", m_Repository, "core");

    READ_OBJECT_LIST(User, "users", m_Users);
    READ_OBJECT_LIST(Group, "groups", m_Groups);

    m_PackageType = PackageType::RLXOS;
    if (data["type"]) {
      m_PackageType = stringToPackageType(data["type"].as<std::string>());
      DEBUG("got type " << data["type"])
    }

    OPTIONAL_VALUE(std::string, "script", m_Script, "");

    m_Node = data;
  }

  std::string const& id() const { return m_ID; }
  std::string const& version() const { return m_Version; }
  std::string const& about() const { return m_About; }
  std::string const& repository() const { return m_Repository; }
  PackageType type() const { return m_PackageType; }

  std::vector<std::string> const& depends() const { return m_Depends; }

  std::vector<User> const& users() const { return m_Users; }
  std::vector<Group> const& groups() const { return m_Groups; }

  std::string const& script() const { return m_Script; };

  YAML::Node const& node() const { return m_Node; }

  std::string file() const {
    DEBUG("package type: " << packageTypeToString(m_PackageType))
    return m_ID + "-" + m_Version + "." + packageTypeToString(m_PackageType);
  }

  void dump(std::ostream& os, bool as_meta = false) const {
    auto prefix = as_meta ? "    " : "";
    if (as_meta) {
      os << "  - id: " << m_ID << "\n";
    } else {
      os << "id: " << m_ID << "\n";
    }

    os << prefix << "version: " << m_Version << "\n"
       << prefix << "about: " << m_About << "\n";

    os << prefix << "repository: " << m_Repository << "\n";

    os << prefix << "type: " << packageTypeToString(m_PackageType) << '\n';

    if (m_Depends.size()) {
      os << prefix << "depends:"
         << "\n";
      for (auto const& i : m_Depends) os << prefix << " - " << i << "\n";
    }

    if (m_Users.size()) {
      os << prefix << "users: " << std::endl;
      for (auto const& i : m_Users) {
        i.dump(os, prefix);
      }
    }

    if (m_Groups.size()) {
      os << prefix << "groups: " << std::endl;
      for (auto const& i : m_Groups) {
        i.dump(os, prefix);
      }
    }

    if (m_Script.size()) {
      os << prefix << "script: \"" << m_Script << "\"" << std::endl;
    }
  }
};

}  // namespace rlxos::libpkgupd

#endif