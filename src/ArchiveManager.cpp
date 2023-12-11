#include "ArchiveManager.h"
#include "Execute.h"

#include <fstream>

void ArchiveManager::get(const std::filesystem::path &filepath, const std::string &input_path,
                         std::string &output) {
    auto [status, out] = Executor("/bin/tar")
            .arg("--zstd")
            .arg("-O")
            .arg("-xPf")
            .arg(filepath)
            .arg(input_path)
            .output();
    if (status != 0) {
        throw std::runtime_error("failed to get data from " + filepath.string());
    }
    output = std::move(out);
}

void ArchiveManager::extract(const std::filesystem::path &filepath,
                             const std::string &input_path,
                             const std::filesystem::path &output_path) {
    std::ofstream writer(output_path);
    std::stringstream error;
    int status = Executor("/bin/tar")
            .arg("--zstd")
            .arg("-O")
            .arg("-xPf")
            .arg(filepath)
            .arg(input_path)
            .start()
            .wait(&writer, &error);
    if (status != 0) {
        throw std::runtime_error("failed to get data from " + filepath.string() + ": " + error.str());
    }
}

MetaInfo ArchiveManager::info(const std::filesystem::path &input_path) {
    std::string content;
    get(input_path, "./info", content);

    return MetaInfo(content);
}

void ArchiveManager::list(const std::filesystem::path &filepath, std::vector<std::string> &files) {
    std::stringstream output;
    std::stringstream error;
    int status = Executor("/bin/tar")
            .arg("--zstd")
            .arg("-tf")
            .arg(filepath)
            .start()
            .wait(&output, &error);
    if (status != 0) {
        throw std::runtime_error("failed to list file from archive " + filepath.string() + ": " + error.str());
    }

    std::stringstream ss(output.str());
    std::string file;

    while (std::getline(ss, file, '\n')) {
        files.push_back(file);
    }
}

void ArchiveManager::extract(const std::filesystem::path &filepath, const std::string &output_path,
                             std::vector<std::string> &files_list) {
    std::stringstream output;
    std::stringstream error;

    int status = Executor("/bin/tar")
            .arg("--zstd")
            .arg("--exclude")
            .arg("./info")
            .arg("-xPhpf")
            .arg(filepath)
            .arg("-C")
            .arg(output_path)
            .start()
            .wait(&output, &error);

    if (!std::filesystem::exists(output_path)) {
        std::error_code code;
        std::filesystem::create_directories(output_path, code);
        if (code) {
            throw std::runtime_error("failed to create required directory '" + output_path + "': " + code.message());
        }
    }
    std::stringstream ss(output.str());
    for (std::string f; std::getline(ss, f);) {
        files_list.emplace_back(f);
    }

    if (status != 0) {
        throw std::runtime_error("failed to execute extraction command");
    }
}

void ArchiveManager::compress(const std::filesystem::path &filepath, const std::string &input_path) {
    auto [status, output] = Executor("/bin/tar")
            .arg("--zstd")
            .arg("-cPf")
            .arg(filepath)
            .arg("-C")
            .arg(input_path)
            .arg(".").output();
    if (status != 0) {
        throw std::runtime_error("failed to execute command for compression: " + output);
    }
}

bool ArchiveManager::is_archive(const std::filesystem::path &filepath) {
    for (auto const &ext: {".tar", ".zip", ".gz", ".xz", ".bzip2", ".tgz", ".txz", ".bz"}) {
        if (filepath.has_extension() && filepath.extension() == ext) {
            return true;
        }
    }
    return false;
}
