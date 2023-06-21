#ifndef LIBPKGUPD_USER
#define LIBPKGUPD_USER

#include <yaml-cpp/yaml.h>

#include "defines.hxx"

namespace rlxos::libpkgupd {
/**
 * @brief Users holds the information about linux
 * system users
 *
 */
    class User {
    private:
        unsigned int m_ID;
        std::string m_Name, m_About, m_Dir, m_Shell, m_Group;

    public:
        /**
         * @brief Construct a new User object from provided parameters
         *
         * @param id user id
         * @param name
         * @param about
         * @param dir
         * @param shell
         * @param group
         */
        User(unsigned int id, std::string const &name, std::string const &about,
             std::string const &dir, std::string const &shell,
             std::string const &group)
                : m_ID(id),
                  m_Name(name),
                  m_About(about),
                  m_Dir(dir),
                  m_Shell(shell),
                  m_Group(group) {}

        /**
         * @brief Construct a new User object from
         * YAML configuration node
         *
         * @param data
         * @param file
         */
        User(YAML::Node const &data, std::string const &file);

        /**
         * @brief return the name of user
         *
         * @return std::string user name
         */
        std::string const &name() const { return m_Name; }

        /**
         * @brief check if user exists or not
         *
         * @return true if user exists
         * @return false
         */
        bool exists() const;

        /**
         * @brief create new user in system database using `useradd` command
         *
         * @return true on success
         * @return false
         */
        bool create() const;

        /**
         * @brief emit the user information in YAML format
         *
         * @param os
         */
        void dump(std::ostream &os) const;
    };
}  // namespace rlxos::libpkgupd

#endif