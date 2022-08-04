#include "../libpkgupd/container.hh"
using namespace rlxos::libpkgupd;

#include <algorithm>
#include <iostream>
using namespace std;

PKGUPD_MODULE_HELP(run) {
  os << "Run binaries inside container" << endl
     << PADDING << "  " << BOLD("Options:") << endl
     << PADDING << "  - run.config=" << BOLD("<path>")
     << "   # Set container configuration" << endl
     << endl;
}

PKGUPD_MODULE(run) {
  auto container = std::make_shared<Container>(config);

  if (!container->run(args, config->get<bool>("debug", false))) {
    ERROR(container->error());
    return 1;
  }
  return 0;
}