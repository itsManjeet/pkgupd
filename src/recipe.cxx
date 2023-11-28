#include "recipe.hxx"

#include <sstream>

#include "defines.hxx"

using std::string;

namespace rlxos::libpkgupd {
    Recipe::Recipe(YAML::Node data, const std::string &file) : mFilePath(file) {
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

        READ_LIST(string, "sources", m_Sources);
        READ_LIST(string, "environ", m_Environ);

        READ_LIST(string, "skip-strip", m_SkipStrip);
        READ_LIST(string, "backup", m_Backup);

        READ_LIST(string, "include", m_Include);

        OPTIONAL_VALUE(bool, "strip", m_DoStrip, true);
        std::string buildType;

        OPTIONAL_VALUE(string, "build-type", buildType, "");

        OPTIONAL_VALUE(string, "configure", m_Configure, "");
        OPTIONAL_VALUE(string, "compile", m_Compile, "");
        OPTIONAL_VALUE(string, "install", m_Install, "");

        OPTIONAL_VALUE(string, "build-dir", m_BuildDir, "");

        OPTIONAL_VALUE(string, "script", m_Script, "");
        OPTIONAL_VALUE(string, "pre-script", m_PreScript, "");
        OPTIONAL_VALUE(string, "post-script", m_PostScript, "");

        OPTIONAL_VALUE(string, "install-script", m_InstallScript, "");

        if (!buildType.empty()) {
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

    void Recipe::dump(std::ostream &os, bool as_meta) const {
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

    std::string Recipe::hash() const {
        std::stringstream ss;
        dump(ss);

        unsigned h = 37;
        auto const str = ss.str();
        for (auto c : str)
            h = (h * 54059) ^ (str[0] * 76963);

        std::stringstream hex;
        hex << std::hex << h;
        return hex.str();
    }
}  // namespace rlxos::libpkgupd