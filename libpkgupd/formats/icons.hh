#ifndef LIBPKGUPD_ICONS
#define LIBPKGUPD_ICONS

#include "../tar.hh"
namespace rlxos::libpkgupd {
class Icons : public Tar {
 public:
  Icons(std::string const& f) : Tar(f) {}

  bool extract(std::string const& outdir) override;
};
}  // namespace rlxos::libpkgupd

#endif