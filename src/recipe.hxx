#ifndef LIBPKGUPD_RECIPE
#define LIBPKGUPD_RECIPE

#include <yaml-cpp/yaml.h>

#include <optional>

#include "builder/builder.hxx"
#include "defines.hxx"
#include "package-info.hxx"

namespace rlxos::libpkgupd {

    class Recipe {
       private:
        std::string mFilePath;
        std::string m_ID, m_Version, m_About;
        std::vector<std::string> m_Depends, m_BuildTime;

        BuildType m_BuildType;

        std::vector<std::string> m_Environ, m_Sources;
        std::string m_BuildDir;
        std::string m_Configure, m_Compile, m_Install;
        std::string m_PreScript, m_PostScript;

        std::vector<std::string> m_SkipStrip;
        bool m_DoStrip;

        std::string m_InstallScript;

        std::string m_Script;
        std::vector<std::string> m_Backup;
        std::vector<std::string> m_Include;

        YAML::Node m_Node;

       public:
        Recipe(YAML::Node data, const std::string &file);

        std::string const &id() const { return m_ID; }

        std::string const &version() const { return m_Version; }

        std::string const &about() const { return m_About; }

        BuildType buildType() const { return m_BuildType; }

        std::vector<std::string> const &depends() const { return m_Depends; }

        std::vector<std::string> const &buildTime() const { return m_BuildTime; }

        std::string const &buildDir() const { return m_BuildDir; }

        std::string const &configure() const { return m_Configure; }

        std::string const &compile() const { return m_Compile; }

        std::string const &install() const { return m_Install; }

        std::vector<std::string> const &include() const { return m_Include; }

        std::string const &prescript() const { return m_PreScript; }

        std::string const &postscript() const { return m_PostScript; }

        std::string const &script() const { return m_Script; }

        std::string const &installScript() const { return m_InstallScript; }

        void setDependency() {}

        std::vector<std::string> const &environ() const { return m_Environ; }

        std::vector<std::string> const &sources() const { return m_Sources; }

        std::vector<std::string> const &skipStrip() const { return m_SkipStrip; }

        std::shared_ptr<PackageInfo> const package() const {
            return std::make_shared<PackageInfo>(m_ID, m_Version, m_About, m_Depends, m_Backup, m_InstallScript,
                                                 hash(),
                                                 m_Node);
        }

        bool dostrip() const { return m_DoStrip; }

        void setStrip(bool s) { m_DoStrip = s; }

        std::string const &filePath() const { return mFilePath; }

        YAML::Node const &node() const { return m_Node; }

        void dump(std::ostream &os, bool as_meta = false) const;

        template <typename T>
        T get(const std::string &key, T def) const {
            return m_Node[key] ? m_Node[key].as<T>() : def;
        }

        std::string hash() const;
    };
}  // namespace rlxos::libpkgupd

#endif