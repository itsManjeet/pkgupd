#ifndef _LIBPKGUPD_STRIPPER_HH_
#define _LIBPKGUPD_STRIPPER_HH_

#include "defines.hxx"

namespace rlxos::libpkgupd {
    class Stripper : public Object {
    private:
        std::string _script;
        std::string _filter = "cat";

    public:
        Stripper(std::vector<std::string> const &skips = {});

        METHOD(std::string, script);

        bool strip(std::string const &dir);
    };
}  // namespace rlxos::libpkgupd

#endif