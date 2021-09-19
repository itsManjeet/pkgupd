#include "Stripper.hh"
#include "Command.hh"

namespace rlxos::libpkgupd
{
    Stripper::Stripper()
    {
        // Thanks to (VenomLinux - scratchpkg) https://github.com/emmeett1
        // https://github.com/venomlinux/scratchpkg/blob/master/pkgbuild#L215

        script = "find . -type f -printf \"%P\\n\" 2>/dev/null | " + filter +
                 " | while read -r binary ; do \n"
                 " case \"$(file -bi \"$binary\")\" in \n"
                 " *application/x-sharedlib*)      strip --strip-unneeded \"$binary\" ;; \n"
                 " *application/x-pie-executable*) strip --strip-unneeded \"$binary\" ;; \n"
                 " *application/x-archive*)        strip --strip-debug    \"$binary\" ;; \n"
                 " *application/x-object*) \n"
                 "    case \"$binary\" in \n"
                 "     *.ko)                       strip --strip-unneeded \"$binary\" ;; \n"
                 "     *)                          continue ;; \n"
                 "    esac;; \n"
                 " *application/x-executable*)     strip --strip-all \"$binary\" ;; \n"
                 " *)                              continue ;; \n"
                 " esac\n"
                 " done\n";
    }

    bool Stripper::Strip(std::string const &dir)
    {
        auto cmd = Command("/bin/sh", {"-e", "-u", "-c", script});
        cmd.SetDirectory(dir);

        if (int status = Command::ExecuteScript(script, dir, {}); status != 0)
        {
            error = "Strip script failed with exit code: " + std::to_string(status);
            return false;
        }
        return true;
    }
}