#ifndef PKGUPD_TRIGGERER_H
#define PKGUPD_TRIGGERER_H

#include "SystemDatabase.h"
#include "defines.h"

class Triggerer {
public:
    enum class type : int {
        INVALID,
        MIME,
        DESKTOP,
        FONTS_SCALE,
        HARDWARE,
        UDEV,
        ICONS,
        GTK3_INPUT_MODULES,
        GTK2_INPUT_MODULES,
        GLIB_SCHEMAS,
        GIO_MODULES,
        GDK_PIXBUF,
        FONTS_CACHE,
        LIBRARY_CACHE,
    };

protected:
    std::string mesg(type t);

    std::string regex(type t);

    type get(std::string const& path);

    std::tuple<bool, std::string> exec(type t);

    std::vector<type> get(std::vector<std::string> const& files_list);

public:
    Triggerer() = default;

    bool trigger(std::vector<std::pair<InstalledMetaInfo,
                    std::vector<std::string>>> const& infos);

    bool trigger();
};

#endif
