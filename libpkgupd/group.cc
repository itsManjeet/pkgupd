#include "group.hh"

#include <grp.h>

#include "defines.hh"
#include "exec.hh"

namespace rlxos::libpkgupd {
Group::Group(YAML::Node const &data, std::string const &file) {
  READ_VALUE(unsigned int, "id", m_ID);
  READ_VALUE(std::string, "name", m_Name);
}

bool Group::exists() const { return getgrnam(m_Name.c_str()) != nullptr; }

bool Group::create() const {
  return Executor::execute("groupadd -g " + std::to_string(m_ID) + " " +
                           m_Name) == 0;
}

void Group::dump(std::ostream &os, std::string prefix) const {
  os << prefix << " - id: " << m_ID << "\n"
     << prefix << "   name: " << m_Name << std::endl;
}

}  // namespace rlxos::libpkgupd
