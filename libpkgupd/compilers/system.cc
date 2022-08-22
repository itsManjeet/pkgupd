#include "system.hh"

#include "../container.hh"
#include "../recipe.hh"
using namespace rlxos::libpkgupd;

#include <sys/wait.h>
#define ERROR std::string(strerror(errno))

bool System::compile(Recipe* recipe, Configuration* config, std::string dir,
                     std::string destdir, std::vector<std::string>& environ) {
  recipe->setStrip(false);
  pid_t pid = fork();
  if (pid == -1) {
    p_Error = "fork() failed: " + ERROR;
    return false;
  }

  if (pid == 0) {
    INFO("chrooting into " << destdir);
    if (chroot(destdir.c_str()) == -1) {
      std::cerr << "chroot() failed: " + std::string(strerror(errno))
                << std::endl;
      exit(EXIT_FAILURE);
    }

    auto get_value = [recipe](std::string var, std::string def) -> std::string {
      return recipe->node()[var] ? recipe->node()[var].as<std::string>() : def;
    };

    if (chdir(get_value("inside.workdir", "/").c_str()) == -1) {
      std::cerr << "chdir() failed: " + std::string(strerror(errno))
                << std::endl;
      exit(EXIT_FAILURE);
    }

    if (config->get("inside.pkgupd.trigger", true)) {
      INFO("triggering pkgupd");
      if (Executor::execute("pkgupd trigger dir.data=/usr/share/" +
                            recipe->id() + "/included") != 0) {
        exit(EXIT_FAILURE);
      }
    }

    if (config->get("inside.pwconv.required", true)) {
      if (Executor::execute("pwconv && grpconv") != 0) {
        exit(EXIT_FAILURE);
      }
    }

    auto root = get_value("inside.root", "");
    if (root.size()) {
      INFO("setting root password");
      if (Executor::execute("echo '" + root + "\n" + root + "' |  passwd") !=
          0) {
        exit(EXIT_FAILURE);
      }
    }

    auto pkg = recipe->packages()[0];
    for (auto const& group : pkg->groups()) {
      INFO("creating new group " << group.name());
      if (!group.create()) {
        std::cerr << "failed to create " + group.name() + " group" << std::endl;
        exit(EXIT_FAILURE);
      }
    }

    for (auto const& user : pkg->users()) {
      INFO("creating new user " << user.name());
      if (!user.create()) {
        std::cerr << "failed to create " + user.name() + " user" << std::endl;
        exit(EXIT_FAILURE);
      }
    }

    std::vector<std::string> systemServices;
    config->get("services.system", systemServices);
    for (auto const& service : systemServices) {
      INFO("enabling system service");
      if (Executor::execute("systemctl enable " + service) != 0) {
        exit(EXIT_FAILURE);
      }
    }

    std::vector<std::string> userServices;
    config->get("services.user", userServices);
    for (auto const& service : userServices) {
      if (Executor::execute("systemctl enable --global " + service) != 0) {
        exit(EXIT_FAILURE);
      }
    }

    std::string inside = get_value("inside", "");
    if (inside.size()) {
      if (Executor::execute(inside, ".", environ) != 0) {
        exit(EXIT_FAILURE);
      }
    }
    exit(EXIT_SUCCESS);
  }
  int status;
  if (waitpid(pid, &status, 0) == -1) {
    p_Error = "waitpid() failed: " + ERROR;
    return false;
  }

  int exitCode = WEXITSTATUS(status);
  if (exitCode != 0) {
    p_Error += "\nprocess exit with: " + std::to_string(exitCode);
    return false;
  }
  return true;
}