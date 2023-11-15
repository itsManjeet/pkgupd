#include "package-info.hxx"

using namespace rlxos::libpkgupd;

PackageInfo::PackageInfo(YAML::Node const &data, std::string const &file) {
    READ_VALUE(std::string, "id", m_ID);
    READ_VALUE(std::string, "version", m_Version);
    READ_VALUE(std::string, "about", m_About);
    READ_LIST(std::string, "depends", m_Depends);
    READ_LIST(std::string, "backup", m_Backup);

    OPTIONAL_VALUE(std::string, "script", m_Script, "");

    m_Node = data;
}

void PackageInfo::dump(std::ostream &os) const {
    os << "id: " << m_ID << "\n";

    os << "version: " << m_Version << "\n"
       << "about: " << m_About << "\n";

    if (m_Depends.size()) {
        os << "depends:"
           << "\n";
        for (auto const &i: m_Depends) os << " - " << i << "\n";
    }

    if (m_Backup.size()) {
        os << "backup:"
           << "\n";
        for (auto const &i: m_Backup) os << " - " << i << "\n";
    }

    if (m_Script.size()) {
        os << "script: |\n";
        std::stringstream ss(m_Script);
        std::string line;
        while (std::getline(ss, line)) {
            os << "  " << line << '\n';
        }
        os << std::endl;
    }

    if (m_Node["extra"]) {
        YAML::Node extra;
        extra["extra"] = m_Node["extra"];
        os << extra << std::endl;
    }
}