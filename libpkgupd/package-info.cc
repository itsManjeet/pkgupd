#include "package-info.hh"
using namespace rlxos::libpkgupd;

PackageInfo::PackageInfo(YAML::Node const& data, std::string const& file) {
  READ_VALUE(std::string, "id", m_ID);
  READ_VALUE(std::string, "version", m_Version);
  READ_VALUE(std::string, "about", m_About);
  READ_LIST(std::string, "depends", m_Depends);
  READ_LIST(std::string, "backup", m_Backup);
  OPTIONAL_VALUE(std::string, "repository", m_Repository, "core");

  READ_OBJECT_LIST(User, "users", m_Users);
  READ_OBJECT_LIST(Group, "groups", m_Groups);
  m_PackageType = PACKAGE_TYPE_FROM_STR(data["type"].as<std::string>().c_str());
  OPTIONAL_VALUE(std::string, "script", m_Script, "");

  m_Node = data;
}

void PackageInfo::dump(std::ostream& os, bool as_meta) const {
  auto prefix = as_meta ? "    " : "";
  if (as_meta) {
    os << "  - id: " << m_ID << "\n";
  } else {
    os << "id: " << m_ID << "\n";
  }

  os << prefix << "version: " << m_Version << "\n"
     << prefix << "about: " << m_About << "\n";

  os << prefix << "repository: " << m_Repository << "\n";

  os << prefix << "type: " << PACKAGE_TYPE_ID[PACKAGE_TYPE_INT(m_PackageType)]
     << '\n';

  if (m_Depends.size()) {
    os << prefix << "depends:"
       << "\n";
    for (auto const& i : m_Depends) os << prefix << " - " << i << "\n";
  }

  if (m_Backup.size()) {
    os << prefix << "backup:"
       << "\n";
    for (auto const& i : m_Backup) os << prefix << " - " << i << "\n";
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

  if (m_Node["extra"]) {
    YAML::Node extra;
    extra["extra"] = m_Node["extra"];
    os << extra << std::endl;
  }
}