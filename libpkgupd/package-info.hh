#ifndef _LIBPKGUPD_PACKAGEINFO_HH_
#define _LIBPKGUPD_PACKAGEINFO_HH_

#include <grp.h>
#include <string.h>
#include <yaml-cpp/yaml.h>

#include <ostream>

#include "defines.hh"
#include "exec.hh"
#include "group.hh"
#include "user.hh"

namespace rlxos::libpkgupd {
#define PACKAGE_TYPE_LIST \
  X(APPIMAGE, "app")      \
  X(PACKAGE, "pkg")       \
  X(THEME, "theme")       \
  X(ICON, "icon")         \
  X(FONT, "font")         \
  X(RLXOS, "rlx")         \
  X(IMAGE, "img")

/**
 * @brief Package type helps to determine the type of packaging and
 * installation methods use for that package
 *
 */
enum class PackageType : int {
#define X(ID, type) ID,
  PACKAGE_TYPE_LIST
#undef X
      N_PACKAGE_TYPE
};

#define PACKAGE_TYPE_INT(pkgtype) static_cast<int>(pkgtype)

static char const*
    PACKAGE_TYPE_STR[PACKAGE_TYPE_INT(PackageType::N_PACKAGE_TYPE)] = {
#define X(ID, type) #ID,
        PACKAGE_TYPE_LIST
#undef X
};

static char const*
    PACKAGE_TYPE_ID[PACKAGE_TYPE_INT(PackageType::N_PACKAGE_TYPE)] = {
#define X(ID, type) type,
        PACKAGE_TYPE_LIST
#undef X
};

#define PACKAGE_TYPE_FROM_STR(str)                                            \
  ({                                                                          \
    PackageType pkgtype = PackageType::N_PACKAGE_TYPE;                        \
    for (int i = 0; i < PACKAGE_TYPE_INT(PackageType::N_PACKAGE_TYPE); i++) { \
      if (!strcmp(PACKAGE_TYPE_ID[i], str)) {                                 \
        pkgtype = PackageType(i);                                             \
        break;                                                                \
      }                                                                       \
    }                                                                         \
    pkgtype;                                                                  \
  })

#define PACKAGE_FILE(pkg)                  \
  pkg->id() + "-" + pkg->version() + "." + \
      std::string(PACKAGE_TYPE_ID[PACKAGE_TYPE_INT(pkg->type())])
/**
 * @brief PackageInfo holds all the information of rlxos packages
 * their dependencies and required user groups
 */
class PackageInfo {
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
  PackageInfo(YAML::Node const& data, std::string const& file);
  PackageInfo(std::string id, std::string version, std::string about,
              std::vector<std::string> depends, PackageType packageType,
              std::vector<User> users, std::vector<Group> groups,
              std::string script, std::string repo, YAML::Node node)
      : m_ID{id},
        m_Version{version},
        m_About{about},
        m_Depends{depends},
        m_PackageType{packageType},
        m_Users{users},
        m_Groups{groups},
        m_Script{script},
        m_Repository{repo},
        m_Node{node} {}
  virtual ~PackageInfo() = default;
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

  void dump(std::ostream& os, bool as_meta = false) const;
};

}  // namespace rlxos::libpkgupd

#endif