#include "Trigger.h"

#include <algorithm>
#include <iostream>
#include <regex>

#include "Executor.h"

Triggerer::type Triggerer::get(std::string const &path) {
    for (auto const &i:
            {
                    type::MIME, type::DESKTOP, type::FONTS_SCALE, type::UDEV, type::ICONS,
                    type::GTK3_INPUT_MODULES, type::GTK2_INPUT_MODULES, type::GLIB_SCHEMAS,
                    type::GIO_MODULES, type::GDK_PIXBUF, type::FONTS_CACHE,
                    type::LIBRARY_CACHE
            }) {
        std::string pattern = regex(i);
        if (path.find(pattern) != std::string::npos) {
            return i;
        }
    }

    return type::INVALID;
}

std::tuple<bool, std::string> Triggerer::exec(type t) {
    std::string cmd;
    std::string error;

    switch (t) {
        case type::MIME:
            cmd = "/bin/update-mime-database /usr/share/mime";
            break;

        case type::DESKTOP:
            cmd = "/bin/update-desktop-database --quiet";
            break;

        case type::UDEV:
            cmd = "/bin/udevadm hwdb --update";
            break;

        case type::GTK3_INPUT_MODULES:
            cmd = "/bin/gtk-query-immodules-3.0 --update-cache";
            break;

        case type::GTK2_INPUT_MODULES:
            cmd = "/bin/gtk-query-immodules-2.0 --update-cache";
            break;

        case type::GLIB_SCHEMAS:
            cmd = "/bin/glib-compile-schemas /usr/share/glib-2.0/schemas";
            break;

        case type::GIO_MODULES:
            cmd = "/bin/gio-querymodules /usr/lib/gio/modules";
            break;

        case type::GDK_PIXBUF:
            cmd = "/bin/gdk-pixbuf-query-loaders --update-cache";
            break;

        case type::FONTS_CACHE:
            cmd = "/bin/fc-cache -s";
            break;

        case type::LIBRARY_CACHE:
            cmd = "/bin/ldconfig";
            break;

        case type::FONTS_SCALE: {
            bool status = true;
            for (auto const &i:
                    std::filesystem::directory_iterator("/usr/share/fonts")) {
                std::error_code ee;
                std::filesystem::remove(i.path() / "fonts.scale", ee);
                std::filesystem::remove(i.path() / "fonts.dir", ee);
                std::filesystem::remove(i.path() / ".uuid", ee);

                if (std::filesystem::is_empty(i.path()))
                    std::filesystem::remove(i.path(), ee);

                if (int status = Executor("/bin/mkfontdir").arg(i.path().string()).run();
                        status != 0) {
                    error +=
                            "\nmkfontdir failed with exit code: " + std::to_string(status);
                    status = false;
                }

                if (int status = Executor("/bin/mkfontscale").arg(i.path().string()).run();
                        status != 0) {
                    error +=
                            "\nmkfontscale failed with exit code: " + std::to_string(status);
                    status = false;
                }
            }

            return {status, error};
        }

        case type::ICONS: {
            bool status = true;
            for (auto const &i:
                    std::filesystem::directory_iterator("/usr/share/icons")) {
                if (int status_code = Executor("/bin/gtk-update-icon-cache").arg("-q").arg(i.path().string()).run();
                        status != 0) {
                    error += "\ngtk-update-icon-cache failed with exit code: " +
                             std::to_string(status_code);
                    status = false;
                }
            }

            return {status, error};
        }
        default:
            throw std::runtime_error("unimplemented trigger executed");
    }
    std::stringstream ss(cmd);
    std::string binary;
    ss >> binary;
    auto executor = Executor(binary);
    for(std::string arg; ss >> arg; ) {
        executor.arg(arg);
    }
    if (int status = executor.run(); status != 0) {
        error += "\nFailed to '" + mesg(t) +
                 "' command failed with exit code: " + std::to_string(status);
        return {false, error};
    }

    return {true, ""};
}

std::string Triggerer::regex(
        type t) {
    // Thanks to venomLinux scratch package manager
    // https://github.com/venomlinux/scratchpkg/blob/master/scratch#L284
    switch (t) {
        case type::MIME:
            return "share/mime/";

        case type::DESKTOP:
            return "share/applications/";

        case type::FONTS_SCALE:
            return "share/fonts/";

        case type::UDEV:
            return "udev/hwdb.d/";

        case type::ICONS:
            return "share/icons/";

        case type::GTK3_INPUT_MODULES:
            return "lib/gtk-3.0/3.0.0/immodules/";

        case type::GTK2_INPUT_MODULES:
            return "lib/gtk-2.0/2.10.0/immodules/";

        case type::GLIB_SCHEMAS:
            return "share/glib-2.0/schemas/";

        case type::GIO_MODULES:
            return "lib/gio/modules/";

        case type::GDK_PIXBUF:
            return "lib/gdk-pixbuf-2.0/2.10.0/loaders/";

        case type::FONTS_CACHE:
            return "share/fonts/";

        case type::LIBRARY_CACHE:
            return "lib/";
    }

    throw std::runtime_error(
            "unimplemented trigger found in triggerer::triggerRegex()");
}

std::string Triggerer::mesg(type t) {
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
        case type::LIBRARY_CACHE:
            return "Updating library cache";
    }

    throw std::runtime_error(
            "unimplemented trigger found in triggerer::getMessage()");
}

std::vector<Triggerer::type> Triggerer::get(
        std::vector<std::string> const &files_list) {
    std::vector<Triggerer::type> requiredTriggers;

    auto checkTrigger = [&](Triggerer::type type) -> bool {
        for (auto const &i: files_list) {
            auto trigger = this->get(i);
            if (trigger == type) {
                return true;
            }
        }

        return false;
    };
    for (auto i:
            {
                    type::MIME, type::DESKTOP, type::FONTS_SCALE, type::UDEV, type::ICONS,
                    type::GTK3_INPUT_MODULES, type::GTK2_INPUT_MODULES, type::GLIB_SCHEMAS,
                    type::GIO_MODULES, type::GDK_PIXBUF, type::FONTS_CACHE,
                    type::LIBRARY_CACHE
            }) {
        if (checkTrigger(i)) {
            requiredTriggers.push_back(i);
        }
    }

    return requiredTriggers;
}

bool Triggerer::trigger(
        std::vector<std::pair<InstalledMetaInfo, std::vector<std::string>>> const &infos) {
    std::vector<type> triggers;
    for (auto const &[info, files_list]: infos) {
        if (!info.integration.empty()) {
            if (int const status = Executor(info.integration).arg(".").environ("VERSION=" + info.version).run();
                    status != 0) {
                ERROR("failed to execute script");
            }
        }
        auto requiredTriggers = get(files_list);
        for (auto i: requiredTriggers) {
            if (std::find(triggers.begin(), triggers.end(), i) == triggers.end()) {
                triggers.push_back(i);
            }
        }
    }

    for (auto const &i: triggers) {
        PROCESS(mesg(i))
        auto [status, error] = exec(i);
        if (!status) {
            ERROR(error);
        }
    }

    return true;
}

bool Triggerer::trigger() {
    for (auto i: {
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
            type::LIBRARY_CACHE,
    }) {
        PROCESS(mesg(i));
        try {
            auto [status, error] = exec(i);
            if (!status) {
                ERROR(error);
            }
        }
        catch (std::exception &e) {
            ERROR(e.what());
        }
    }

    return true;
}