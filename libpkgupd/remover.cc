#include "remover.hh"

namespace rlxos::libpkgupd {
bool Remover::remove(std::shared_ptr<SystemDatabase::package> pkginfo_) {
  auto files = pkginfo_->files();

  bool status = true;

  _files_list.push_back(std::vector<std::string>());

  for (auto i = files.rbegin(); i != files.rend(); i++) {
    std::error_code err;
    std::string path;

    if ((*i).rfind("./", 0) == 0)
      path = _root_dir + "/" + (*i).substr(2, (*i).length() - 2);
    else
      path = _root_dir + "/" + *i;

    if (std::filesystem::is_directory(path)) {
      _files_list.back().push_back(path);

      if (std::filesystem::is_empty(path))
        std::filesystem::remove_all(path, err);
    } else {
      DEBUG("removing " << path);
      std::filesystem::remove(path, err);
      _files_list.back().push_back(path);
    }

    if (err) {
      _error += "\n" + err.message();
      status = false;
    }
  }

  if (!_sys_db.remove(pkginfo_)) {
    _error += _sys_db.error();
    status = false;
  }

  return status;
}

bool Remover::remove(std::vector<std::string> const &pkgs, bool skip_triggers) {
  std::vector<std::shared_ptr<SystemDatabase::package>> pkgsInfo;

  for (auto const &i : pkgs) {
    auto pkginfo_ = _sys_db[i];
    if (pkginfo_ == nullptr) {
      _error = _sys_db.error();
      return false;
    }

    pkgsInfo.push_back(std::dynamic_pointer_cast<SystemDatabase::package>(pkginfo_));
  }

  for (auto const &i : pkgsInfo) {
    PROCESS("cleaning file of " << i->id());
    if (!remove(i)) ERROR(_error);
  }

  if (!skip_triggers) {
    if (!_triggerer.trigger(_files_list)) {
      _error = _triggerer.error();
      return false;
    }
  }

  return true;
}
}  // namespace rlxos::libpkgupd