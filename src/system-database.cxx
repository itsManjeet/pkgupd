#include "system-database.hxx"

#include <string.h>
#include <sys/stat.h>

#include <algorithm>
#include <ctime>
#include <fstream>

#include "defines.hxx"

namespace rlxos::libpkgupd {

    InstalledPackageInfo::InstalledPackageInfo(YAML::Node const &data,
                                               char const *file)
        : PackageInfo(data, file) {
        READ_VALUE(std::string, "installed_on", mInstalledon);
        OPTIONAL_VALUE(bool, "is-dependency", m_IsDependency, false);
    }

    bool SystemDatabase::init() {
        mPackages.clear();

        PROCESS("Reading System Database");
        DEBUG("LOCATION " << data_dir);

        if (!std::filesystem::exists(data_dir)) {
            std::error_code error;
            std::filesystem::create_directories(data_dir, error);
            if (error) {
                p_Error = "Failed to create missing database directory " + data_dir;
                return false;
            }
        }
        for (auto const &file : std::filesystem::directory_iterator(data_dir)) {
            if (file.path().has_extension()) continue;
            try {
                mPackages[file.path().filename().string()] =
                    std::make_shared<InstalledPackageInfo>(
                        YAML::LoadFile(file.path().string()), file.path().c_str());
            } catch (std::exception const &exception) {
                std::cerr << "failed to load: " << exception.what() << std::endl;
            }
        }
        DEBUG("DATABASE SIZE " << mPackages.size());
        return true;
    }

    bool SystemDatabase::get_files(std::shared_ptr<InstalledPackageInfo> packageInfo,
                                   std::vector<std::string> &files) {
        auto files_path =
            std::filesystem::path(data_dir) / (packageInfo->id() + ".files");
        if (!std::filesystem::exists(files_path)) {
            p_Error = "no files data exists for " + packageInfo->id() + " at " +
                      files_path.string();
            return false;
        }

        std::ifstream reader(files_path.string());
        if (!reader.good()) {
            p_Error = "failed to read files data " + files_path.string();
            return false;
        }

        std::string filepath;
        while (std::getline(reader, filepath)) {
            files.push_back(filepath);
        }

        return true;
    }

    std::shared_ptr<InstalledPackageInfo> SystemDatabase::get(char const *pkgid) {
        auto iter = mPackages.find(pkgid);
        if (iter == mPackages.end()) {
            return nullptr;
        }
        return iter->second;
    }

    std::shared_ptr<InstalledPackageInfo>
    SystemDatabase::add(std::shared_ptr<PackageInfo> pkginfo,
                        std::vector<std::string> const &files,
                        std::string root, bool update,
                        bool is_dependency) {
        auto data_file = std::filesystem::path(data_dir) / pkginfo->id();
        if (!std::filesystem::exists(data_dir)) {
            std::error_code code;
            if (!std::filesystem::create_directories(data_dir, code)) {
                p_Error = "failed to create required data directory, " + code.message();
                return nullptr;
            }
        }

        std::ofstream file(data_file);
        if (!file.is_open()) {
            p_Error = "failed to open data file to write at " + data_file.string();
            return nullptr;
        }

        pkginfo->dump(file);

        file << "is-dependency: " << (is_dependency ? "true" : "false") << "\n";

        std::time_t t = std::time(0);
        std::tm *now = std::localtime(&t);

        file << "installed_on: " << (now->tm_year + 1900) << "/" << (now->tm_mon + 1)
             << "/" << (now->tm_mday) << " " << (now->tm_hour) << ":" << (now->tm_min)
             << std::endl;

        file.close();

        mPackages[pkginfo->id()] = std::make_shared<InstalledPackageInfo>(pkginfo.get());
        std::ofstream files_writer(data_file.string() + ".files");
        for (auto const &i : files) {
            files_writer << i << std::endl;
        }
        files_writer.close();

        return mPackages[pkginfo->id()];
    }

    bool SystemDatabase::remove(char const *id) {
        auto data_file = std::filesystem::path(data_dir) / id;
        std::error_code code;
        if (!std::filesystem::exists(data_file)) {
            return true;
        }

        std::filesystem::remove(data_file, code);
        if (code) {
            p_Error = "failed to remove data file: " + data_file.string() + ", " +
                      code.message();
            return false;
        }

        // auto iter = mPackages.find(id);
        // if (iter == mPackages.end()) {
        //   return true;
        // }
        // mPackages.erase(iter);
        return true;
    }
}  // namespace rlxos::libpkgupd