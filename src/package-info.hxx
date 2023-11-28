#ifndef _LIBPKGUPD_PACKAGEINFO_HH_
#define _LIBPKGUPD_PACKAGEINFO_HH_

#include <grp.h>
#include <string.h>
#include <yaml-cpp/yaml.h>

#include <ostream>

#include "defines.hxx"
#include "exec.hxx"

namespace rlxos::libpkgupd {
#define PACKAGE_FILE(pkg) \
    pkg->id() + "-" + pkg->version() + "-" + pkg->cache() + ".pkg"

    /**
     * @brief PackageInfo holds all the information of rlxos packages
     * their dependencies and required user groups
     */
    class PackageInfo {
       protected:
        std::string m_ID;
        std::string m_Version;
        std::string m_About;

        std::vector<std::string> m_Depends;
        std::string m_Cache;

        std::string m_Script;

        std::vector<std::string> m_Backup;

        bool m_IsDependency = false;

        YAML::Node m_Node;

       public:
        PackageInfo(YAML::Node const &data, std::string const &file);

        PackageInfo(std::string id, std::string version, std::string about,
                    std::vector<std::string> depends,
                    std::vector<std::string> backup, std::string script,
                    std::string cache, YAML::Node node)
            : m_ID{id},
              m_Version{version},
              m_About{about},
              m_Depends{depends},
              m_Script{script},
              m_Backup{backup},
              m_Cache{cache},
              m_Node{node} {}

        virtual ~PackageInfo() = default;

        std::string const &id() const { return m_ID; }

        std::string const &version() const { return m_Version; }

        std::string const &about() const { return m_About; }

        std::vector<std::string> const &depends() const { return m_Depends; }

        std::vector<std::string> const &backups() const { return m_Backup; }

        std::string const &cache() const { return m_Cache; }

        std::string const &script() const { return m_Script; };

        YAML::Node const &node() const { return m_Node; }

        YAML::Node &node() { return m_Node; }

        bool isDependency() const { return m_IsDependency; }

        void setDependency() { m_IsDependency = true; }

        void unsetDependency() { m_IsDependency = false; }

        void dump(std::ostream &os) const;
    };

}  // namespace rlxos::libpkgupd

#endif