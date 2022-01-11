#include "triggerer.hh"

#include <algorithm>
#include <iostream>
#include <regex>

#include "exec.hh"

namespace rlxos::libpkgupd {

Triggerer::type Triggerer::_get(std::string const &path) {
  for (auto const &i :
       {type::MIME, type::DESKTOP, type::FONTS_SCALE, type::UDEV, type::ICONS,
        type::GTK3_INPUT_MODULES, type::GTK2_INPUT_MODULES, type::GLIB_SCHEMAS,
        type::GIO_MODULES, type::GDK_PIXBUF, type::FONTS_CACHE}) {
    std::string pattern = _regex(i);
    if (std::regex_match(path, std::regex(pattern))) return i;
  }

  return type::INVALID;
}

bool Triggerer::_exec(type t) {
  std::string cmd;

  switch (t) {
    case type::MIME:
      cmd = "update-mime-database /usr/share/mime";
      break;

    case type::DESKTOP:
      cmd = "update-desktop-database --quiet";
      break;

    case type::UDEV:
      cmd = "udevadm hwdb --update";
      break;

    case type::GTK3_INPUT_MODULES:
      cmd = "gtk-query-immodules-3.0 --update-cache";
      break;

    case type::GTK2_INPUT_MODULES:
      cmd = "gtk-query-immodules-2.0 --update-cache";
      break;

    case type::GLIB_SCHEMAS:
      cmd = "glib-compile-schemas /usr/share/glib-2.0/schemas";
      break;

    case type::GIO_MODULES:
      cmd = "gio-querymodules /usr/lib/gio/modules";
      break;

    case type::GDK_PIXBUF:
      cmd = "gdk-pixbuf-query-loaders --update-cache";
      break;

    case type::FONTS_CACHE:
      cmd = "fc-cache -s";
      break;

    case type::FONTS_SCALE: {
      bool status = true;
      for (auto const &i :
           std::filesystem::directory_iterator("/usr/share/fonts")) {
        std::error_code ee;
        std::filesystem::remove(i.path() / "fonts.scale", ee);
        std::filesystem::remove(i.path() / "fonts.dir", ee);
        std::filesystem::remove(i.path() / ".uuid", ee);

        if (std::filesystem::is_empty(i.path()))
          std::filesystem::remove(i.path(), ee);

        if (int status = Executor().execute("mkfontdir " + i.path().string());
            status != 0) {
          _error +=
              "mkfontdir failed with exit code: " + std::to_string(status);
          status = false;
        }

        if (int status = Executor().execute("mkfontscale " + i.path().string());
            status != 0) {
          _error +=
              "mkfontscale failed with exit code: " + std::to_string(status);
          status = false;
        }
      }

      return status;
    }

    case type::ICONS: {
      bool status = true;
      for (auto const &i :
           std::filesystem::directory_iterator("/usr/share/icons")) {
        if (int status =
                Executor().execute("gtk-update-icon-cache -q " + i.path().string());
            status != 0) {
          _error += "gtk-update-icon-cahce failed with exit code: " +
                    std::to_string(status);
          status = false;
        }
      }

      return status;
    }
    default:
      throw std::runtime_error("unimplemented trigger executed");
  }

  if (int status = Executor().execute(cmd); status != 0) {
    _error += "trigger failed with exit code: " + std::to_string(status);
    status = false;
  }

  return true;
}

std::string Triggerer::_regex(
    type t) {  // Thanks to venomLinux scratch package manager
  // https://github.com/venomlinux/scratchpkg/blob/master/scratch#L284
  switch (t) {
    case type::MIME:
      return "share/mime/$";

    case type::DESKTOP:
      return "share/applications/$";

    case type::FONTS_SCALE:
      return "share/fonts/$";

    case type::UDEV:
      return "udev/hwdb.d/$";

    case type::ICONS:
      return "share/icons/$";

    case type::GTK3_INPUT_MODULES:
      return "lib/gtk-3.0/3.0.0/immodules/.*.so";

    case type::GTK2_INPUT_MODULES:
      return "lib/gtk-2.0/2.10.0/immodules/.*.so";

    case type::GLIB_SCHEMAS:
      return "share/glib-2.0/schemas/$";

    case type::GIO_MODULES:
      return "lib/gio/modules/.*.so";

    case type::GDK_PIXBUF:
      return "lib/gdk-pixbuf-2.0/2.10.0/loaders/.*.so";

    case type::FONTS_CACHE:
      return "share/fonts/$";
  }

  throw std::runtime_error(
      "unimplemented trigger found in triggerer::triggerRegex()");
}

std::string Triggerer::_mesg(type t) {
  // Thanks to venomLinux scratch package manager
  // https://github.com/venomlinux/scratchpkg/blob/master/scratch#L284
  switch (t) {
    case type::MIME:
      return "Updating MIME database";
    case type::DESKTOP:
      return "Updating desktop file MIME type cache";
    case type::FONTS_SCALE:
      return "Updating X fontdir indices";
    case type::UDEV:
      return "Updating hardware database";
    case type::ICONS:
      return "Updating icon caches";
    case type::GTK3_INPUT_MODULES:
      return "Probing GTK3 input method modules";
    case type::GTK2_INPUT_MODULES:
      return "Probing GTK2 input method modules";
    case type::GLIB_SCHEMAS:
      return "Compiling GSettings XML schema files";
    case type::GIO_MODULES:
      return "Updating GIO module cache";
    case type::GDK_PIXBUF:
      return "Probing GDK Pixbuf loader modules";
    case type::FONTS_CACHE:
      return "Updating fontconfig cache";
  }

  throw std::runtime_error(
      "unimplemented trigger found in triggerer::getMessage()");
}

std::vector<Triggerer::type> Triggerer::_get(
    std::vector<std::vector<std::string>> const &fileslist) {
  std::vector<Triggerer::type> requiredTriggers;
  for (auto i :
       {type::MIME, type::DESKTOP, type::FONTS_SCALE, type::UDEV, type::ICONS,
        type::GTK3_INPUT_MODULES, type::GTK2_INPUT_MODULES, type::GLIB_SCHEMAS,
        type::GIO_MODULES, type::GDK_PIXBUF, type::FONTS_CACHE}) {
    for (auto const &pkgfiles : fileslist) {
      if (std::find(requiredTriggers.begin(), requiredTriggers.end(), i) ==
          requiredTriggers.end())
        break;

      for (auto const &file : pkgfiles) {
        auto trigger = _get(file);
        if (trigger != type::INVALID) {
          requiredTriggers.push_back(trigger);
          break;
        }
      }
    }
  }

  return requiredTriggers;
}

bool Triggerer::trigger(std::vector<Package> const &pkgs) {
  bool status = true;
  for (auto const &i : pkgs) {
    for (auto const &grp : i.groups()) {
      PROCESS("creating group " + grp.name());
      if (!grp.exists()) {
        if (!grp.create()) {
          ERROR("failed to create " + grp.name() + " group");
          status = false;
        }
      }
    }
    for (auto const &usr : i.users()) {
      PROCESS("creating user " + usr.name());
      if (!usr.exists()) {
        if (!usr.create()) {
          ERROR("failed to create " + usr.name() + " user");
          status = false;
        }
      }
    }
  }

  return status;
}

bool Triggerer::trigger(
    std::vector<std::vector<std::string>> const &fileslist) {
  if (fileslist.size() == 0) return true;

  bool status = true;
  auto requiredTriggers = _get(fileslist);
  for (auto const &i : requiredTriggers) {
    PROCESS(_mesg(i))

    if (!_exec(i)) {
      _error += "\n" + _error;
      status = false;
    }
  }

  {
    PROCESS("Updating library cache");

    if (int status = Executor().execute("/bin/ldconfig"); status != 0) {
      _error = "failed to update library cache";
      return false;
    }
  }
  return status;
}

bool Triggerer::trigger() {
  for (auto i : {
           type::MIME,
           type::DESKTOP,
           type::FONTS_SCALE,
           type::UDEV,
           type::ICONS,
           type::GTK3_INPUT_MODULES,
           type::GTK2_INPUT_MODULES,
           type::GLIB_SCHEMAS,
           type::GIO_MODULES,
           type::GDK_PIXBUF,
           type::FONTS_CACHE,
       }) {
    PROCESS(_mesg(i));
    try {
      if (!_exec(i)) {
        ERROR(_error);
      }
    } catch (std::exception &e) {
      ERROR(e.what());
    }
  }

  return true;
}
}  // namespace rlxos::libpkgupd