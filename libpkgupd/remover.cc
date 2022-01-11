#include "remover.hh"

namespace rlxos::libpkgupd {
bool Remover::remove(Package const &package) {
  auto files_node = package.node()["files"];
  std::vector<std::string> files;
  for (auto const &i : files_node) {
    files.push_back(i.as<std::string>());
  }

  bool status = true;

  m_FilesList.push_back(std::vector<std::string>());

  for (auto i = files.rbegin(); i != files.rend(); i++) {
    std::error_code err;
    std::string path;

    if ((*i).rfind("./", 0) == 0)
      path = m_RootDir + "/" + (*i).substr(2, (*i).length() - 2);
    else
      path = m_RootDir + "/" + *i;

    if (std::filesystem::is_directory(path)) {
      m_FilesList.back().push_back(path);

      if (std::filesystem::is_empty(path))
        std::filesystem::remove_all(path, err);
    } else {
      DEBUG("removing " << path);
      std::filesystem::remove(path, err);
      m_FilesList.back().push_back(path);
    }

    if (err) {
      p_Error += "\n" + err.message();
      status = false;
    }
  }

  if (!m_SystemDatabase.unregisterFromSystem(package)) {
    p_Error += m_SystemDatabase.error();
    status = false;
  }

  return status;
}

bool Remover::remove(std::vector<std::string> const &pkgs, bool skip_triggers) {
  std::vector<Package> pkgsInfo;

  for (auto const &i : pkgs) {
    auto package = m_SystemDatabase[i];
    if (!package) {
      p_Error = m_SystemDatabase.error();
      return false;
    }

    pkgsInfo.push_back(*package);
  }

  for (auto const &i : pkgsInfo) {
    PROCESS("cleaning file of " << i.id());
    if (!remove(i)) ERROR(p_Error);
  }

  if (!skip_triggers) {
    if (!m_Triggerer.trigger(m_FilesList)) {
      p_Error = m_Triggerer.error();
      return false;
    }
  }

  return true;
}
}  // namespace rlxos::libpkgupd