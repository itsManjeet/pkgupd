#include "tar.hh"

#include <fstream>

namespace rlxos::libpkgupd {

std::tuple<int, std::string> tar::getdata(std::string const &filepath) {
  std::string cmd = "/bin/tar";
  cmd += " --zstd -O -xPf " + _pkgfile + " " + filepath;

  auto [status, output] = exec().output(cmd);
  if (status != 0) {
    _error = "failed to get data from " + _pkgfile;
    return {status, output};
  }
  return {status, output};
}

std::shared_ptr<tar::package> tar::info() {
  auto [status, content] = getdata("./info");
  if (status != 0) {
    _error = "failed to read package information";
    return nullptr;
  }

  DEBUG("info: " << content);
  YAML::Node data;

  try {
    data = YAML::Load(content);
  } catch (YAML::Exception const &e) {
    _error = "corrupt package data, " + std::string(e.what());
    return nullptr;
  }

  return std::make_shared<tar::package>(data, _pkgfile);
}

std::vector<std::string> tar::list() {
  std::string cmd = "/bin/tar";
  cmd += " --zstd -tPf " + _pkgfile;

  auto [status, output] = exec().output(cmd);
  if (status != 0) {
    _error = output;
    return {};
  }

  std::stringstream ss(output);
  std::string file;

  std::vector<std::string> files_list;

  while (std::getline(ss, file, '\n')) files_list.push_back(file);

  return files_list;
}

bool tar::compress(std::string const &srcdir,
                   std::shared_ptr<pkginfo> const &info) {
  std::string pardir = std::filesystem::path(_pkgfile).parent_path();
  if (!std::filesystem::exists(pardir)) {
    std::error_code err;
    std::filesystem::create_directories(pardir, err);
    if (err) {
      _error = "failed to create " + pardir + ", " + err.message();
      return false;
    }
  }

  std::ofstream fileptr(srcdir + "/info");

  fileptr << "id: " << info->id() << "\n"
          << "version: " << info->version() << "\n"
          << "about: " << info->about() << "\n";

  if (info->depends(false).size()) {
    fileptr << "depends:"
            << "\n";
    for (auto const &i : info->depends(false)) fileptr << " - " << i << "\n";
  }

  if (info->users().size()) {
    fileptr << "users: " << std::endl;
    for (auto const &i : info->users()) {
      i->print(fileptr);
    }
  }

  if (info->groups().size()) {
    fileptr << "groups: " << std::endl;
    for (auto const &i : info->groups()) {
      i->print(fileptr);
    }
  }

  if (info->install_script().size()) {
    fileptr << "install_script: | " << std::endl;
    std::stringstream ss(info->install_script());
    std::string line;
    while (std::getline(ss, line, '\n')) fileptr << "  " << line << std::endl;
  }

  fileptr.close();

  std::string command = "/bin/tar";

  command += " --zstd -cPf " + _pkgfile + " -C " + srcdir + " . ";
  if (exec().execute(command) != 0) {
    _error = "failed to execute command for compression '" + command + "'";
    return false;
  }

  return true;
}

bool tar::extract(std::string const &outdir) {
  if (!std::filesystem::exists(_pkgfile)) {
    _error = "no " + _pkgfile + " exist";
    return false;
  }

  std::string cmd = "/bin/tar";

  cmd += " --zstd --exclude './info' -xPhpf " + _pkgfile + " -C " + outdir;
  if (exec().execute(cmd) != 0) {
    _error = "failed to execute extraction command";
    return false;
  }

  return true;
}
}  // namespace rlxos::libpkgupd
