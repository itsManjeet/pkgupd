#ifndef _LIBPKGUPD_STRIPPER_HH_
#define _LIBPKGUPD_STRIPPER_HH_

#include "Defines.hh"

namespace rlxos::libpkgupd
{
    class Stripper : public Object
    {
    private:
        std::string script;
        std::string filter = "cat";

    public:
        Stripper();

        void SetSkip(std::vector<std::string> const &skips)
        {
            if (skips.size())
            {
                filter = "grep -v";
                for (auto const &i : skips)
                    filter += " -e " + i.substr(0, i.length() - 1);
            }
        }

        void SetScript(std::string const &script)
        {
            this->script = script;
        }

        bool Strip(std::string const& dir);
    };
}

#endif