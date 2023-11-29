#include "ArchiveManager.hxx"
#include "../exec.hxx"

using namespace rlxos::libpkgupd;

void ArchiveManager::get(const std::filesystem::path& filepath, const std::string& input_path,
                         std::string& output) {
    std::string cmd = "/bin/tar";
    cmd += " --zstd -O -xPf";
    cmd += filepath.string();
    cmd += " ";
    cmd += input_path;

    auto [status, out] = Executor::output(cmd);
    if (status != 0) {
        throw std::runtime_error("failed to get data from " + filepath.string());
    }
    output = std::move(out);
}

void ArchiveManager::extract(const std::filesystem::path& filepath,
                             const std::string& input_path,
                             const std::filesystem::path& output_path) {
    std::string cmd = "/bin/tar";
    cmd += " --zstd -O -xPf";
    cmd += filepath;
    cmd += " ";
    cmd += " ";
    cmd += input_path;
    cmd += " >";
    cmd += output_path;

    auto [status, out] = Executor::output(cmd);
    if (status != 0) {
        throw std::runtime_error("failed to get data from " + filepath.string() + ": " + out);
    }
}

MetaInfo ArchiveManager::info(const std::filesystem::path& input_path) {
    std::string content;
    get(input_path, "./info", content);

    return MetaInfo(YAML::Load(content));
}

void ArchiveManager::list(const std::filesystem::path& input_path, std::vector<std::string>& files) {
    std::string cmd = "/bin/tar";
    cmd += " --zstd -tPf ";
    cmd += input_path;

    auto [status, output] = Executor::output(cmd);
    if (status != 0) {
        throw std::runtime_error("failed to list file from archive " + input_path.string() + ": " + output);
    }

    std::stringstream ss(output);
    std::string file;

    while (std::getline(ss, file, '\n')) {
        files.push_back(file);
    }
}

void ArchiveManager::extract(const std::filesystem::path& filepath, const std::string& output_path,
                             std::vector<std::string>&) {
    std::string cmd = "/bin/tar";
    cmd += " --zstd";
    cmd += " --exclude './info'";
    cmd += " -xPhpf ";
    cmd += filepath;
    cmd += " -C ";
    cmd += output_path;

    if (!std::filesystem::exists(output_path)) {
        std::error_code code;
        std::filesystem::create_directories(output_path, code);
        if (code) {
            throw std::runtime_error("failed to create required directory '" + output_path + "': " + code.message());
        }
    }

    if (Executor::execute(cmd) != 0) {
        throw std::runtime_error("failed to execute extraction command");
    }
}

void ArchiveManager::compress(const std::filesystem::path& filepath, const std::string& input_path) {
    std::string cmd = "/bin/tar";
    cmd += " --zstd -cPf ";
    cmd += filepath;
    cmd += " -C ";
    cmd += input_path;
    cmd += " . ";

    if (Executor::execute(cmd) != 0) {
        throw std::runtime_error("failed to execute command for compression '" + cmd + "'");
    }
}
