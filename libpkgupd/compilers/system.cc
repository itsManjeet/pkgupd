#include "system.hh"

#include "../container.hh"
#include "../recipe.hh"
using namespace rlxos::libpkgupd;

bool System::compile(Recipe* recipe, Configuration* config, std::string dir,
                     std::string destdir, std::vector<std::string>& environ) {
  YAML::Node container_config;

  container_config["run.inside"] = destdir;
  container_config["run.proc"] = true;
  container_config["run.clone"] = std::vector<std::string>{
      "ns",
      "pid",
  };

  if (recipe->node()["container"]) {
    container_config = recipe->node()["container"];
  }
  std::string shell = "sh";
  if (recipe->node()["shell"]) {
    shell = recipe->node()["shell"].as<std::string>();
  }

  auto local_config = Configuration(container_config);

  auto container = Container(&local_config);
  // set root password
  if (config->node()["root"]) {
    PROCESS("setting up root password");
    if (!container.run({shell, "-c",
                        "echo \"" + config->node()["root"].as<std::string>() +
                            "\" | passwd --stdin"})) {
      p_Error = "failed to set root password: " + container.error();
      return false;
    }
  }

  // enable services
  std::vector<std::string> services;
  config->get("services", services);
  for (auto const& service : services) {
    PROCESS("enabling service " << service);
    if (!container.run({"systemd", "enable", service}, true)) {
      p_Error =
          "failed to enable service '" + service + "', " + container.error();
      return false;
    }
  }

  // execute script
  if (recipe->node()["inside"]) {
    PROCESS("executing script");
    if (!container.run(
            {shell, "-c", recipe->node()["inside"].as<std::string>()}, true)) {
      p_Error = container.error();
      return false;
    }
  }

  return true;
}