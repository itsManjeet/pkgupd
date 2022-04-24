#ifndef LIBPKGUPD_FONTS
#define LIBPKGUPD_FONTS

#include "../tar.hh"
namespace rlxos::libpkgupd {
class Fonts : public Tar {
 public:
  Fonts(std::string const& f) : Tar(f) {}

  bool extract(std::string const& outdir) override;
};
}  // namespace rlxos::libpkgupd

#endif