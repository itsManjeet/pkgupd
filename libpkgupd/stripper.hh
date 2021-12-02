#ifndef _LIBPKGUPD_STRIPPER_HH_
#define _LIBPKGUPD_STRIPPER_HH_

#include "defines.hh"

namespace rlxos::libpkgupd {
class stripper : public object {
 private:
  std::string _script;
  std::string _filter = "cat";

 public:
  stripper(std::vector<std::string> const &skips = {});

  METHOD(std::string, script);

  bool strip(std::string const &dir);
};
}  // namespace rlxos::libpkgupd

#endif