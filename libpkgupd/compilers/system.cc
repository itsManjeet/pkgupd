#include "system.hh"

#include "../container.hh"
#include "../recipe.hh"
using namespace rlxos::libpkgupd;

#include <sys/wait.h>
#define ERROR std::string(strerror(errno))

bool System::compile(Recipe* recipe, Configuration* config, std::string dir,
                     std::string destdir, std::vector<std::string>& environ) {
  pid_t pid = fork();
  if (pid == -1) {
    p_Error = "fork() failed: " + ERROR;
    return false;
  }

  if (pid == 0) {
    INFO("chrooting into " << destdir);
    if (chroot(destdir.c_str()) == -1) {
      p_Error = "chroot() failed: " + std::string(strerror(errno));
      return false;
    }

    if (chdir(config->get<std::string>("inside.workdir", "/").c_str()) == -1) {
      p_Error = "chdir() failed: " + std::string(strerror(errno));
      return false;
    }

    auto pkg = recipe->packages()[0];
    for (auto const& group : pkg->groups()) {
      INFO("creating new group " << group.name());
      if (!group.create()) {
        p_Error = "failed to create " + group.name() + " group";
        return false;
      }
    }

    for (auto const& user : pkg->users()) {
      INFO("creating new user " << user.name());
      if (!user.create()) {
        p_Error = "failed to create " + user.name() + " user";
        return false;
      }
    }

    std::vector<std::string> systemServices;
    config->get("services.system", systemServices);
    for (auto const& service : systemServices) {
      INFO("enabling system service");
      auto [status, output] = Executor::output("systemctl enable " + service);
      if (!status) {
        p_Error = "failed to enable systemctl service " + output;
        return false;
      }
    }

    std::vector<std::string> userServices;
    config->get("services.user", userServices);
    for (auto const& service : userServices) {
      auto [status, output] =
          Executor::output("systemctl enable --global " + service);
      if (!status) {
        p_Error = "failed to enable systemctl service " + output;
        return false;
      }
    }

    {
      auto [status, output] = Executor::output(
          config->get<std::string>("inside", ""), ".", environ);
      if (!status) {
        p_Error = "failed to executed inside script " + output;
        return false;
      }
    }

    exit(EXIT_SUCCESS);
  }
  int status;
  if (waitpid(pid, &status, 0) == -1) {
    p_Error = "waitpid() failed: " + ERROR;
    return false;
  }

  if (WIFEXITED(status)) {
    int exitCode = WEXITSTATUS(status);
    return true;
  }

  return true;
}