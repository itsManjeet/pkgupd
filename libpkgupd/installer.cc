#include "installer.hh"

#include <cassert>
#include <iostream>

#include "archive.hh"
#include "exec.hh"
#include "image.hh"
#include "tar.hh"
#include "triggerer.hh"

namespace rlxos::libpkgupd {

bool installer::_install(std::vector<std::string> const &pkgs,
                         std::string const &root_dir,
                         bool skip_triggers,
                         bool force) {
    std::vector<std::shared_ptr<pkginfo>> pkginfo_list;
    std::vector<std::vector<std::string>> all_pkgs_fileslist;

    for (auto const &i : pkgs) {
        std::shared_ptr<archive> archive_;
        std::string ext = std::filesystem::path(i).extension();
        if (ext == ".rlx") {
            archive_ = std::make_shared<tar>(i);
        } else if (ext == ".app") {
            archive_ = std::make_shared<image>(i);
        } else {
            _error = "unsupport package of type '" + ext + "'";
            return false;
        }

        DEBUG("getting information from " << std::filesystem::path(i).filename().string());

        auto info = archive_->info();
        if (info == nullptr) {
            _error = archive_->error();
            return false;
        }

        try {
            if (!force) {
                if (_sysdb.is_installed(info) && !_sysdb.outdated(info)) {
                    INFO(info->id() + " latest version is already installed")
                    continue;
                }
            }
        } catch (...) {
        }

        if (root_dir == "/") {
            PROCESS("installing " << BLUE(info->id()));
        } else {
            PROCESS("installing " << BLUE(info->id()) << " into " << RED(root_dir));
        }

        if (!archive_->extract(root_dir)) {
            _error = archive_->error();
            return false;
        }
        all_pkgs_fileslist.push_back(archive_->list());
        pkginfo_list.push_back(info);
    }

    assert(all_pkgs_fileslist.size() == pkginfo_list.size());

    bool _with_pkgname = pkginfo_list.size() != 1;

    for (int i = 0; i < pkginfo_list.size(); i++) {
        if (_with_pkgname) {
            PROCESS("registering into database " << BLUE(pkginfo_list[i]->id()));
        } else {
            PROCESS("registering into database");
        }

        if (!_sysdb.add(pkginfo_list[i], all_pkgs_fileslist[i], root_dir, force)) {
            _error = _sysdb.error();
            return false;
        }

        if (!skip_triggers) {
            if (pkginfo_list[i]->install_script().size()) {
                if (_with_pkgname) {
                    PROCESS("post install script " << BLUE(pkginfo_list[i]->id()));
                } else {
                    PROCESS("post installation script");
                }

                if (int status = exec().execute(pkginfo_list[i]->install_script()); status != 0) {
                    _error = "install script failed to exit code: " + std::to_string(status);
                    return false;
                }
            }
        }
    }

    if (skip_triggers) {
        INFO("skipping triggers")
    } else {
        auto triggerer_ = triggerer();
        if (!triggerer_.trigger(all_pkgs_fileslist)) {
            _error = triggerer_.error();
            return false;
        }
    }

    if (skip_triggers) {
        INFO("skipped creating users account")
    } else {
        auto triggerer_ = triggerer();
        if (!triggerer_.trigger(pkginfo_list)) {
            _error = triggerer_.error();
            return false;
        }
    }

    return true;
}
bool installer::install(std::vector<std::string> const &pkgs,
                        std::string const &root_dir,
                        bool skip_triggers,
                        bool force) {
    std::vector<std::string> archive_list;

    for (auto const &i : pkgs) {
        if (std::filesystem::exists(i)) {
            archive_list.push_back(i);
            continue;
        }

        auto info = _repodb[i];
        if (info == nullptr) {
            _error = "no package found with name '" + i + "'";
            return false;
        }

        if (_sysdb.is_installed(info) && !force) {
            _error = info->id() + " is already installed in the system";
            return false;
        }

        auto pkgfile = info->packagefile();
        auto pkgpath = _pkg_dir + "/" + pkgfile;

        if (!std::filesystem::exists(pkgpath)) {
            PROCESS("getting " << pkgfile);
            if (!_downloader.get(pkgfile, pkgpath)) {
                _error = _downloader.error();
                return false;
            }
        }

        archive_list.push_back(pkgpath);
    }

    return _install(archive_list, root_dir, skip_triggers, force);
}
}  // namespace rlxos::libpkgupd