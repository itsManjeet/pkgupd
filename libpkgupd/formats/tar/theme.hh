#ifndef LIBPKGUPD_THEMES
#define LIBPKGUPD_THEMES

#include "../tar.hh"
namespace rlxos::libpkgupd {
class Themes : public Tar {
 public:
  Themes(std::string const& f) : Tar(f) {}

  bool extract(std::string const& outdir) override;
};
}  // namespace rlxos::libpkgupd

#endif