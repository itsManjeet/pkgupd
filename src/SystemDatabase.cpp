#include "SystemDatabase.h"

#include "defines.hxx"
#include <algorithm>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sys/stat.h>
#include <utility>

InstalledMetaInfo::InstalledMetaInfo(const std::string& input) {
    update_from_data(input, "");

    // timestamp = config.get<std::string>("timestamp");
}

std::string InstalledMetaInfo::str() const {
    std::stringstream ss;
    ss << MetaInfo::str() << '\n';
    ss << "timestamp: " << timestamp << '\n'
       << "dependency: " << (dependency ? "true" : "false") << std::endl;

    return ss.str();
}

void SystemDatabase::load(std::filesystem::path p) {
    data_dir = std::move(p);
    mPackages.clear();

    PROCESS("Reading System Database");
    DEBUG("LOCATION " << data_dir);

    if (!std::filesystem::exists(data_dir)) {
        std::error_code error;
        std::filesystem::create_directories(data_dir, error);
        if (error) {
            throw std::runtime_error(
                    "Failed to create missing database directory " +
                    data_dir.string());
        }
    }
    for (auto const& path : std::filesystem::directory_iterator(data_dir)) {
        auto info_file_path = path.path() / "info";
        if (!std::filesystem::exists(info_file_path)) continue;
        try {
            std::ifstream reader(info_file_path);
            auto meta_info = InstalledMetaInfo(
                    std::string((std::istreambuf_iterator<char>(reader)),
                            (std::istreambuf_iterator<char>())));
            mPackages[meta_info.id] = std::move(meta_info);
            ;
        } catch (std::exception const& exception) {
            std::cerr << "failed to load: " << exception.what() << std::endl;
        }
    }
    DEBUG("DATABASE SIZE " << mPackages.size());
}

void SystemDatabase::get_files(const InstalledMetaInfo& installed_meta_info,
        std::vector<std::string>& files) const {
    auto files_path = std::filesystem::path(data_dir) /
                      installed_meta_info.name() / "files";
    if (!std::filesystem::exists(files_path)) {
        throw std::runtime_error("no files data exists for " +
                                 installed_meta_info.id + " at " +
                                 files_path.string());
    }

    std::ifstream reader(files_path.string());
    if (!reader.good()) {
        throw std::runtime_error(
                "failed to read files data " + files_path.string());
    }

    std::string filepath;
    while (std::getline(reader, filepath)) { files.push_back(filepath); }
}

std::optional<InstalledMetaInfo> SystemDatabase::get(
        const std::string& id) const {
    auto const iter = mPackages.find(id);
    if (iter == mPackages.end()) { return std::nullopt; }
    return iter->second;
}

InstalledMetaInfo SystemDatabase::add(const MetaInfo& meta_info,
        std::vector<std::string> const& files, const std::string& root,
        bool update, bool is_dependency) {
    PROCESS("Registering " << meta_info.id);
    DEBUG("NAME  : " << meta_info.name());
    DEBUG("FILES : " << files.size())
    auto installed_data_path =
            std::filesystem::path(data_dir) / meta_info.name();
    if (!std::filesystem::exists(installed_data_path)) {
        if (std::error_code code; !std::filesystem::create_directories(
                    installed_data_path, code)) {
            throw std::runtime_error(
                    "failed to create required data directory, " +
                    code.message());
        }
    }

    std::stringstream timestamp;
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    timestamp << (now->tm_year + 1900) << "/" << (now->tm_mon + 1) << "/"
              << (now->tm_mday) << " " << (now->tm_hour) << ":"
              << (now->tm_min);

    auto const installed_meta_info =
            InstalledMetaInfo(meta_info, timestamp.str(), is_dependency);
    {
        std::ofstream writer(installed_data_path / "info");
        if (!writer.is_open()) {
            throw std::runtime_error("failed to open data file to write at " +
                                     (installed_data_path / "info").string());
        }
        writer << installed_meta_info.str();
    }

    mPackages[installed_meta_info.id] = installed_meta_info;
    {
        std::ofstream writer(installed_data_path / "files");
        for (auto const& i : files) { writer << i << std::endl; }
    }

    return installed_meta_info;
}

void SystemDatabase::remove(const InstalledMetaInfo& installed_meta_info) {
    auto const data_file =
            std::filesystem::path(data_dir) / installed_meta_info.name();
    if (std::filesystem::exists(data_file)) {
        std::error_code code;
        std::filesystem::remove(data_file, code);
        if (code)
            throw std::runtime_error(
                    "failed to remove data file: " + data_file.string() + ": " +
                    code.message());
    }

    if (auto const iter = mPackages.find(installed_meta_info.id);
            iter == mPackages.end()) {
        mPackages.erase(iter);
    }
}
