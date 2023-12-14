#ifndef LIBPKGUPD_ENGINE_HXX
#define LIBPKGUPD_ENGINE_HXX

#include "SystemDatabase.h"
#include "Repository.h"
#include "Downloader.h"
#include "Builder.h"
#include "Container.h"


class Engine {

private:
    const Configuration &config;
    SystemDatabase system_database;
    Repository repository;
    const std::filesystem::path root;
    const std::string server;

public:
    explicit Engine(const Configuration &config);

    void load_system_database();

    InstalledMetaInfo install(const MetaInfo &meta_infos, std::vector<std::string> &deprecated_files);

    std::filesystem::path build(const Builder::BuildInfo &build_info, const std::optional<Container>& container = {});

    std::filesystem::path hash(const Builder::BuildInfo &build_info);

    void uninstall(const InstalledMetaInfo &installed_meta_infos);

    [[nodiscard]] std::map<std::string, InstalledMetaInfo> const &list_installed() const;

    void sync(bool force = false);

    void resolve(const std::vector<std::string> &ids, std::vector<MetaInfo> &output_meta_infos) const;

    void resolve(const std::vector<MetaInfo> &ids, std::vector<MetaInfo> &output_meta_infos) const;

    void list_installed_files(const InstalledMetaInfo &installed_meta_info, std::vector<std::string> &files_list);

    [[nodiscard]] std::filesystem::path download(const MetaInfo &meta_info, bool force) const;

    [[nodiscard]] MetaInfo get_remote_meta_info(const std::string &id) const;

    [[nodiscard]] std::map<std::string, MetaInfo> const &list_remote() const;

    void triggers(const std::vector<InstalledMetaInfo> &installed_meta_infos) const;

    void triggers() const;
};

#endif //LIBPKGUPD_ENGINE_HXX
