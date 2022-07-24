#ifndef LIBPKGUPD_CONTAINER_HH
#define LIBPKGUPD_CONTAINER_HH

#include "configuration.hh"
#include "defines.hh"
namespace rlxos::libpkgupd {
// Container wrapper around bubble wrap
class Container : public Object {
 private:
  // mConfig holds the configuration
  Configuration* mConfig;

 public:
  Container(Configuration* config) : mConfig{config} {}
  
  bool run(std::vector<std::string> args, bool debug = false);
};
}  // namespace rlxos::libpkgupd

#endif