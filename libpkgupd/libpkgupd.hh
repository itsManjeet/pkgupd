#ifndef _LIBPKGUPD_HH_
#define _LIBPKGUPD_HH_

#include <filesystem>

#include "builder.hh"
#include "colors.hh"
#include "installer.hh"
#include "packager.hh"
#include "recipe.hh"
#include "remover.hh"
#include "resolver.hh"

namespace rlxos::libpkgupd {

struct UpdateInformation {
  Package previous;
  Package updated;
};
enum class ListType : int {
  Installed,
  Available,
};

class Pkgupd : public Object {
 private:
  SystemDatabase m_SystemDatabase;
  Repository m_Repository;
  Installer m_Installer;
  Remover m_Remover;
  Downloader m_Downloader;

  Resolver m_Resolver;
  Triggerer m_Triggerer;

  std::string m_RootDir;

  std::string m_PackageDir, m_SourceDir, m_BuildDir;

  bool m_IsForce;
  bool m_IsSkipTriggers;

 public:
  Pkgupd(std::string const &dataPath, std::string const &cachePath,
         std::vector<std::string> const &mirrors,
         std::vector<std::string> const &repos, std::string const &version,
         std::string const &rootsPath, bool isForce, bool isSkipTriggers)
      : m_SystemDatabase(dataPath),
        m_PackageDir(cachePath + "/pkgs"),
        m_SourceDir(cachePath + "/src"),
        m_Repository(cachePath + "/recipes", repos),
        m_Installer(m_SystemDatabase, m_Repository, m_Downloader,
                    cachePath + "/pkgs"),
        m_RootDir(rootsPath),
        m_Remover(m_SystemDatabase, rootsPath),
        m_Resolver(m_SystemDatabase, m_Repository),
        m_IsForce(isForce),
        m_IsSkipTriggers(isSkipTriggers),
        m_Downloader(mirrors, version) {
    auto pkgupd_dir = std::filesystem::path(cachePath);
  }

  static std::shared_ptr<Pkgupd> from_config(std::string const &configFile) {
    YAML::Node config = YAML::LoadFile(configFile);

    std::string systemDatabasePath = "/var/lib/pkgupd/data";
    std::string cachePath = "/var/cache/pkgupd/";
    std::string rootDir = "/";
    std::string version = "2200";

    std::vector<std::string> mirrors = {"https://rlxos.dev/storage"};
    std::vector<std::string> repositories = {"core"};

    auto getConfig = [&](std::string id, std::string fallback) -> std::string {
      if (config[id]) {
        DEBUG("using '" << id << "' : " << config[id].as<std::string>());
        return config[id].as<std::string>();
      }
      return fallback;
    };

    systemDatabasePath = getConfig("SystemDatabase", systemDatabasePath);
    cachePath = getConfig("CachePath", cachePath);
    rootDir = getConfig("RootDir", rootDir);
    version = getConfig("Version", version);

    if (config["Mirrors"]) {
      mirrors.clear();
      for (auto const &i : config["Mirrors"]) {
        DEBUG("got mirror " << i.as<std::string>());
        mirrors.push_back(i.as<std::string>());
      }
    }

    if (config["Repositories"]) {
      repositories.clear();
      for (auto const &i : config["Repositories"]) {
        DEBUG("got repo " << i.as<std::string>());
        repositories.push_back(i.as<std::string>());
      }
    }

    if (config["EnvironmentVariables"]) {
      for (auto const &i : config["EnvironmentVariables"]) {
        auto ev = i.as<std::string>();
        auto idx = ev.find_first_of('=');
        if (idx == std::string::npos) {
          ERROR("invalid EnvironmentVariable: " << ev);
          continue;
        }

        DEBUG("setting " << ev)
        setenv(ev.substr(0, idx).c_str(),
               ev.substr(idx + 1, ev.length() - (idx + 1)).c_str(), 1);
      }
    }

    return std::make_shared<Pkgupd>(systemDatabasePath, cachePath, mirrors,
                                    repositories, version, rootDir, false,
                                    false);
  }

  ~Pkgupd() {}

  bool install(std::vector<std::string> const &packages);

  bool build(std::string recipefile);

  bool remove(std::vector<std::string> const &packages);

  bool update(std::vector<std::string> const &packages);

  bool isInstalled(std::string const &pkgid);

  bool genSync(std::string const &path, std::string const &id, std::string const& repo, bool source);

  std::optional<Package> info(std::string packageName);

  std::vector<UpdateInformation> outdate();

  std::tuple<std::vector<std::string>, bool> depends(
      std::vector<std::string> const &package, bool all = false);

  std::vector<Package> search(std::string query);

  std::vector<Package> list(ListType listType);

  bool trigger(std::vector<std::string> const &packages);

  bool sync();
};
}  // namespace rlxos::libpkgupd
#endif