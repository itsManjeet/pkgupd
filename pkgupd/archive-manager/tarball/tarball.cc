#include "tarball.hh"

#include "../../utils/utils.hh"

using namespace rlxos::libpkgupd;

#include <string>
using namespace std;

bool TarBall::get(char const* tarfile, char const* input_path,
                  std::string& output) {
  string cmd = "/bin/tar";
  cmd += " --zstd -O -xPf";
  cmd += tarfile;
  cmd += " ";
  cmd += input_path;

  auto [status, out] = Executor::output(cmd);
  if (status != 0) {
    p_Error = "failed to get data from " + string(input_path);
    return false;
  }
  output = out;
  return true;
}

bool TarBall::extract_file(char const* tarfile, char const* input_path,
                           char const* output_path) {
  string cmd = "/bin/tar";
  cmd += " --zstd -O -xPf";
  cmd += tarfile;
  cmd += " ";
  cmd += " ";
  cmd += input_path;
  cmd += " >";
  cmd += output_path;

  auto [status, out] = Executor::output(cmd);
  if (status != 0) {
    p_Error = "failed to get data from " + string(input_path) + ", " + out;
    return false;
  }
  return true;
}

shared_ptr<PackageInfo> TarBall::info(char const* input_path) {
  std::string content;
  if (!get(input_path, "./info", content)) {
    return nullptr;
  }

  shared_ptr<PackageInfo> pkginfo = nullptr;
  try {
    auto node = YAML::Load(content);
    pkginfo = make_shared<PackageInfo>(node, input_path);
  } catch (exception const& exc) {
    p_Error = exc.what();
  }
  return pkginfo;
}

bool TarBall::list(char const* input_path, vector<string>& files) {
  string cmd = "/bin/tar";
  cmd += " --zstd -tPf ";
  cmd += input_path;

  auto [status, output] = Executor::output(cmd);
  if (status != 0) {
    p_Error = output;
    return false;
  }

  stringstream ss(output);
  string file;

  while (getline(ss, file, '\n')) {
    files.push_back(file);
  };
  return true;
}

bool TarBall::extract(char const* input_path, char const* output_path,
                      std::vector<std::string>&) {
  std::string cmd = "/bin/tar";
  cmd += " --zstd";
  cmd += " --exclude './info'";
  cmd += " -xPhpf ";
  cmd += input_path;
  cmd += " -C ";
  cmd += output_path;

  if (!std::filesystem::exists(output_path)) {
    std::error_code code;
    std::filesystem::create_directories(output_path, code);
    if (code) {
      p_Error = "failed to create required directory '" +
                std::string(output_path) + "'";
      return false;
    }
  }

  if (Executor::execute(cmd) != 0) {
    p_Error = "failed to execute extraction command";
    return false;
  }

  return true;
}

bool TarBall::compress(char const* input_path, char const* src_dir) {
  std::string cmd = "/bin/tar";
  cmd += " --zstd -cPf ";
  cmd += input_path;
  cmd += " -C ";
  cmd += src_dir;
  cmd += " . ";

  if (Executor::execute(cmd) != 0) {
    p_Error = "failed to execute command for compression '" + cmd + "'";
    return false;
  }

  return true;
}