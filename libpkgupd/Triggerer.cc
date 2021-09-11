#include "Triggerer.hh"
#include "Command.hh"

#include <iostream>
#include <algorithm>
#include <regex>

namespace rlxos::libpkgupd
{

    Triggerer::TriggerType Triggerer::getTrigger(std::string path)
    {
        for (auto const &i : {TriggerType::MIME, TriggerType::DESKTOP, TriggerType::FONTS_SCALE,
                              TriggerType::UDEV, TriggerType::ICONS, TriggerType::GTK3_INPUT_MODULES,
                              TriggerType::GTK2_INPUT_MODULES, TriggerType::GLIB_SCHEMAS, TriggerType::GIO_MODULES,
                              TriggerType::GDK_PIXBUF, TriggerType::FONTS_CACHE})
        {
            std::string regexPattern = triggerRegex(i);
            if (std::regex_match(path, std::regex(regexPattern)))
                return i;
        }

        return TriggerType::INVALID;
    }

    bool Triggerer::executeTrigger(TriggerType triggerType)
    {
        std::string binary;
        std::vector<std::string> args;

        switch (triggerType)
        {
        case TriggerType::MIME:
            binary = "/bin/update-mime-database";
            args = {"/usr/share/mime"};
            break;

        case TriggerType::DESKTOP:
            binary = "/bin/update-desktop-database";
            args = {"--quiet"};
            break;

        case TriggerType::UDEV:
            binary = "/bin/udevadm";
            args = {"hwdb", "--update"};
            break;

        case TriggerType::GTK3_INPUT_MODULES:
            binary = "/bin/gtk-query-immodules-3.0";
            args = {"--update-cache"};
            break;

        case TriggerType::GTK2_INPUT_MODULES:
            binary = "/bin/gtk-query-immodules-2.0";
            args = {"--update-cache"};
            break;

        case TriggerType::GLIB_SCHEMAS:
            binary = "/bin/glib-compile-scheams";
            args = {"/usr/share/glib-2.0/schemas"};
            break;

        case TriggerType::GIO_MODULES:
            binary = "/bin/gio-querymodules";
            args = {"/usr/lib/gio/modules"};
            break;

        case TriggerType::GDK_PIXBUF:
            binary = "/bin/gdk-pixbuf-query-loaders";
            args = {"--update-cache"};
            break;

        case TriggerType::FONTS_CACHE:
            binary = "/bin/fc-cache";
            args = {"-s"};
            break;

        case TriggerType::FONTS_SCALE:
        {
            bool status = true;
            for (auto const &i : std::filesystem::directory_iterator("/usr/share/fonts"))
            {
                std::filesystem::remove(i.path() / "fonts.scale");
                std::filesystem::remove(i.path() / "fonts.dir");
                std::filesystem::remove(i.path() / ".uuid");

                if (std::filesystem::is_empty(i.path()))
                    std::filesystem::remove(i.path());

                binary = "/bin/mkfontdir";
                args = {i.path().string()};
                {
                    auto cmd = Command(binary, args);
                    if (int status = cmd.Execute(); status != 0)
                    {
                        error += "mkfontdir failed with exit code: " + std::to_string(status);
                        status = false;
                    }
                }

                binary = "/bin/mkfontscale";
                args = {i.path().string()};
                {
                    auto cmd = Command(binary, args);
                    if (int status = cmd.Execute(); status != 0)
                    {
                        error += "mkfontscale failed with exit code: " + std::to_string(status);
                        status = false;
                    }
                }
            }

            return status;
        }

        case TriggerType::ICONS:
        {
            bool status = true;
            for (auto const &i : std::filesystem::directory_iterator("/usr/share/icons"))
            {

                binary = "/bin/gtk-update-icon-cache";
                args = {"-q", i.path().string()};
                auto cmd = Command(binary, args);
                if (int status = cmd.Execute(); status != 0)
                {
                    error += "gtk-update-icon-cahce failed with exit code: " + std::to_string(status);
                    status = false;
                }
            }

            return status;
        }
        default:
            throw std::runtime_error("unimplemented trigger executed");
        }

        auto cmd = Command(binary, args);
        if (int status = cmd.Execute(); status != 0)
        {
            error += binary + " failed with exit code: " + std::to_string(status);
            status = false;
        }

        return true;
    }

    std::string Triggerer::triggerRegex(TriggerType triggerType)
    { // Thanks to venomLinux scratch package manager
        // https://github.com/venomlinux/scratchpkg/blob/master/scratch#L284
        switch (triggerType)
        {
        case TriggerType::MIME:
            return "^./usr/share/mime/$";

        case TriggerType::DESKTOP:
            return "^./usr/share/applications/$";

        case TriggerType::FONTS_SCALE:
            return "^./usr/share/fonts/$";

        case TriggerType::UDEV:
            return "^./etc/udev/hwdb.d/$";

        case TriggerType::ICONS:
            return "^./usr/share/icons/$";

        case TriggerType::GTK3_INPUT_MODULES:
            return "^./usr/lib/gtk-3.0/3.0.0/immodules/.*.so";

        case TriggerType::GTK2_INPUT_MODULES:
            return "^./usr/lib/gtk-2.0/2.10.0/immodules/.*.so";

        case TriggerType::GLIB_SCHEMAS:
            return "^./usr/share/glib-2.0/schemas/$";

        case TriggerType::GIO_MODULES:
            return "^./usr/lib/gio/modules/.*.so";

        case TriggerType::GDK_PIXBUF:
            return "^./usr/lib/gdk-pixbuf-2.0/2.10.0/loaders/.*.so";

        case TriggerType::FONTS_CACHE:
            return "^./usr/share/fonts/$";
        }

        throw std::runtime_error("unimplemented trigger found in Triggerer::triggerRegex()");
    }

    std::string Triggerer::triggerMessage(TriggerType triggerType)
    {
        // Thanks to venomLinux scratch package manager
        // https://github.com/venomlinux/scratchpkg/blob/master/scratch#L284
        switch (triggerType)
        {
        case TriggerType::MIME:
            return "Updating MIME database";
        case TriggerType::DESKTOP:
            return "Updating desktop file MIME type cache";
        case TriggerType::FONTS_SCALE:
            return "Updating X fontdir indices";
        case TriggerType::UDEV:
            return "Updating hardware database";
        case TriggerType::ICONS:
            return "Updating icon caches";
        case TriggerType::GTK3_INPUT_MODULES:
            return "Probing GTK3 input method modules";
        case TriggerType::GTK2_INPUT_MODULES:
            return "Probing GTK2 input method modules";
        case TriggerType::GLIB_SCHEMAS:
            return "Compiling GSettings XML schema files";
        case TriggerType::GIO_MODULES:
            return "Updating GIO module cache";
        case TriggerType::GDK_PIXBUF:
            return "Probing GDK Pixbuf loader modules";
        case TriggerType::FONTS_CACHE:
            return "Updating fontconfig cache";
        }

        throw std::runtime_error("unimplemented trigger found in Triggerer::getMessage()");
    }

    std::vector<Triggerer::TriggerType> Triggerer::getTrigger(std::vector<std::vector<std::string>> const &fileslist)
    {
        std::vector<Triggerer::TriggerType> requiredTriggers;
        for (auto i : {TriggerType::MIME, TriggerType::DESKTOP, TriggerType::FONTS_SCALE,
                       TriggerType::UDEV, TriggerType::ICONS, TriggerType::GTK3_INPUT_MODULES,
                       TriggerType::GTK2_INPUT_MODULES, TriggerType::GLIB_SCHEMAS, TriggerType::GIO_MODULES,
                       TriggerType::GDK_PIXBUF, TriggerType::FONTS_CACHE})
        {
            for (auto const &pkgfiles : fileslist)
            {
                if (std::find(requiredTriggers.begin(), requiredTriggers.end(), i) == requiredTriggers.end())
                    break;

                for (auto const &file : pkgfiles)
                {
                    auto trigger = getTrigger(file);
                    if (trigger != TriggerType::INVALID)
                    {
                        requiredTriggers.push_back(trigger);
                        break;
                    }
                }
            }
        }

        return requiredTriggers;
    }

    bool Triggerer::Trigger(std::vector<std::vector<std::string>> const &fileslist)
    {
        if (fileslist.size() == 0)
            return true;
            
        bool status = true;
        auto requiredTriggers = getTrigger(fileslist);
        for (auto const &i : requiredTriggers)
        {
            std::cout << triggerMessage(i) << std::endl;
            if (!executeTrigger(i))
            {
                error += "\n" + error;
                status = false;
            }
        }

        {

            std::cout << "Updating library cache" << std::endl;
            // Executing ldconfig
            auto cmd = Command("/bin/ldconfig", {});
            if (int status = cmd.Execute(); status != 0)
            {
                error = "ldconfig failed";
                return false;
            }
        }
        return status;
    }
}