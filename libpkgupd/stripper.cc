#include "stripper.hh"

#include "exec.hh"

namespace rlxos::libpkgupd {
stripper::stripper(std::vector<std::string> const &skips) {
  // Thanks to (VenomLinux - scratchpkg) https://github.com/emmeett1
  // https://github.com/venomlinux/scratchpkg/blob/master/pkgbuild#L215

  if (skips.size()) {
    _filter = "grep -v";
    for (auto const &i : skips) _filter += " -e " + i.substr(0, i.length() - 1);
  }

  _script =
      "find . -type f -printf \"%P\\n\" 2>/dev/null | " + _filter +
      " | while read -r binary ; do \n"
      " case \"$(file -bi \"$binary\")\" in \n"
      " *application/x-sharedlib*)      strip --strip-unneeded \"$binary\" ;; "
      "\n"
      " *application/x-pie-executable*) strip --strip-unneeded \"$binary\" ;; "
      "\n"
      " *application/x-archive*)        strip --strip-debug    \"$binary\" ;; "
      "\n"
      " *application/x-object*) \n"
      "    case \"$binary\" in \n"
      "     *.ko)                       strip --strip-unneeded \"$binary\" ;; "
      "\n"
      "     *)                          continue ;; \n"
      "    esac;; \n"
      " *application/x-executable*)     strip --strip-all \"$binary\" ;; \n"
      " *)                              continue ;; \n"
      " esac\n"
      " done\n";
}

bool stripper::strip(std::string const &dir) {
  if (int status = exec().execute(_script, dir); status != 0) {
    _error = "strip script failed with exit code: " + std::to_string(status);
    return false;
  }
  return true;
}
}  // namespace rlxos::libpkgupd