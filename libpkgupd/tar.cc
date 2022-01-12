#include "tar.hh"

#include <fstream>

namespace rlxos::libpkgupd {

std::tuple<int, std::string> Tar::get(std::string const &filepath) {
  std::string cmd = "/bin/tar";
  cmd += " --zstd -O -xPf " + m_PackageFile + " " + filepath;

  auto [status, output] = Executor().output(cmd);
  if (status != 0) {
    p_Error = "failed to get data from " + m_PackageFile;
    return {status, output};
  }
  return {status, output};
}

std::optional<Package> Tar::info() {
  auto [status, content] = get("./info");
  if (status != 0) {
    p_Error = "failed to read package information";
    return {};
  }

  DEBUG("info: " << content);
  YAML::Node data;

  try {
    data = YAML::Load(content);
  } catch (YAML::Exception const &e) {
    p_Error = "corrupt package data, " + std::string(e.what());
    return {};
  }

  return Package(data, m_PackageFile);
}

std::vector<std::string> Tar::list() {
  std::string cmd = "/bin/tar";
  cmd += " --zstd -tPf " + m_PackageFile;

  auto [status, output] = Executor().output(cmd);
  if (status != 0) {
    p_Error = output;
    return {};
  }

  std::stringstream ss(output);
  std::string file;

  std::vector<std::string> files_list;

  while (std::getline(ss, file, '\n')) files_list.push_back(file);

  return files_list;
}

bool Tar::compress(std::string const &srcdir, Package const &package) {
  std::string pardir = std::filesystem::path(m_PackageFile).parent_path();
  if (!std::filesystem::exists(pardir)) {
    std::error_code err;
    std::filesystem::create_directories(pardir, err);
    if (err) {
      p_Error = "failed to create " + pardir + ", " + err.message();
      return false;
    }
  }

  std::string command = "/bin/tar";

  command += " --zstd -cPf " + m_PackageFile + " -C " + srcdir + " . ";
  if (Executor().execute(command) != 0) {
    p_Error = "failed to execute command for compression '" + command + "'";
    return false;
  }

  return true;
}

bool Tar::extract(std::string const &outdir) {
  if (!std::filesystem::exists(m_PackageFile)) {
    p_Error = "no " + m_PackageFile + " exist";
    return false;
  }

  std::string cmd = "/bin/tar";

  cmd += " --zstd --exclude './info' -xPhpf " + m_PackageFile + " -C " + outdir;
  if (Executor().execute(cmd) != 0) {
    p_Error = "failed to execute extraction command";
    return false;
  }

  return true;
}
}  // namespace rlxos::libpkgupd
