#include "installer.hh"

#include <cassert>
#include <iostream>

#include "archive.hh"
#include "triggerer.hh"

namespace rlxos::libpkgupd {

bool installer::_install(std::vector<std::string> const &pkgs,
                         std::string const &root_dir,
                         bool skip_triggers,
                         bool force) {
    std::vector<std::shared_ptr<pkginfo>> pkginfo_list;
    std::vector<std::vector<std::string>> all_pkgs_fileslist;

    for (auto const &i : pkgs) {
        auto archive_ = archive(i);

        PROCESS("Getting information from " << std::filesystem::path(i).filename().string());

        auto info = archive_.info();
        if (info == nullptr) {
            _error = archive_.error();
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

        PROCESS("extracting " << info->id() << " into " << root_dir);
        if (!archive_.extract(root_dir)) {
            _error = archive_.error();
            return false;
        }
        all_pkgs_fileslist.push_back(archive_.list());
        pkginfo_list.push_back(info);
    }

    assert(all_pkgs_fileslist.size() == pkginfo_list.size());

    for (int i = 0; i < pkginfo_list.size(); i++) {
        PROCESS("Registering " << pkginfo_list[i]->id() << " into system database");
        if (!_sysdb.add(pkginfo_list[i], all_pkgs_fileslist[i], root_dir, force)) {
            _error = _sysdb.error();
            return false;
        }
    }

    if (skip_triggers) {
        INFO("Skipping Triggers")
    } else {
        auto triggerer_ = triggerer();
        if (!triggerer_.trigger(all_pkgs_fileslist)) {
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