#include "../libpkgupd/container.hh"
using namespace rlxos::libpkgupd;

#include <algorithm>
#include <iostream>
using namespace std;

PKGUPD_MODULE_HELP(run) {
  os << "Run binaries inside container" << endl
     << PADDING << "  " << BOLD("Options:") << endl
     << PADDING << "  - run.config=" << BOLD("<path>")
     << "   # Set container configuration" << endl;
}

PKGUPD_MODULE(run) {
  YAML::Node container_node;
  if (config->node()["run.config"]) {
    container_node = YAML::LoadFile(config->get<std::string>("run.config", ""));
  } else if (config->node()["container"]) {
    container_node = config->node()["container"];
  }

  auto container_config = std::make_shared<Configuration>(container_node);
  auto container = std::make_shared<Container>(container_config.get());

  if (!container->run(args, config->get<bool>("debug", false))) {
    ERROR(container->error());
    return 1;
  }
  return 0;
}