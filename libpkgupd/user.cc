#include "user.hh"

#include <pwd.h>

#include "exec.hh"

namespace rlxos::libpkgupd {

User::User(YAML::Node const &data, std::string const &file) {
  READ_VALUE(unsigned int, "id", m_ID);
  READ_VALUE(std::string, "name", m_Name);
  READ_VALUE(std::string, "about", m_About);
  READ_VALUE(std::string, "group", m_Group);
  READ_VALUE(std::string, "dir", m_Dir);
  READ_VALUE(std::string, "shell", m_Shell);
}

bool User::exists() const { return getpwnam(m_Name.c_str()) != nullptr; }

bool User::create() const {
  return Executor().execute("useradd -c '" + m_About + "' -d " + m_Dir +
                            " -u " + std::to_string(m_ID) + " -g " + m_Group +
                            " -s " + m_Shell + " " + m_Name) == 0;
}

void User::dump(std::ostream &os, std::string prefix) const {
  os << prefix << " - id: " << m_ID << "\n"
     << prefix << "   name: " << m_Name << "\n"
     << prefix << "   about: " << m_About << "\n"
     << prefix << "   group: " << m_Group << "\n"
     << prefix << "   dir: " << m_Dir << "\n"
     << prefix << "   shell: " << m_Shell << std::endl;
}

}  // namespace rlxos::libpkgupd
