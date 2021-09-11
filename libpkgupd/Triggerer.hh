#ifndef _PKGUPD_TRIGGERER_HH_
#define _PKGUPD_TRIGGERER_HH_

#include "Defines.hh"

namespace rlxos::libpkgupd
{
    class Triggerer : public Object
    {
    private:
        enum class TriggerType : int
        {
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
        };

        std::string triggerMessage(TriggerType triggerType);

        std::string triggerRegex(TriggerType triggerType);

        TriggerType getTrigger(std::string path);

        bool executeTrigger(TriggerType triggerType);

        std::vector<TriggerType> getTrigger(std::vector<std::vector<std::string>> const &fileslist);

    public:
        Triggerer()
        {
        }
        bool Trigger(std::vector<std::vector<std::string>> const &fileslist);
    };
}

#endif