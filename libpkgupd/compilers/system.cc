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

    if (chdir(recipe->get<std::string>("inside.workdir", "/").c_str()) == -1) {
      std::cerr << "chdir() failed: " + std::string(strerror(errno))
                << std::endl;
      exit(EXIT_FAILURE);
    }

    if (recipe->get<bool>("inside.pkgupd.trigger", true)) {
      INFO("triggering pkgupd");
      if (Executor::execute("pkgupd trigger dir.data=" +
                            recipe->get<std::string>("inside.pkgupd.data",
                                                     "/var/lib/pkgupd/data") +
                            recipe->id() + "/included") != 0) {
        exit(EXIT_FAILURE);
      }
    }

    if (recipe->get<bool>("inside.pwconv.required", true)) {
      if (Executor::execute("pwconv && grpconv") != 0) {
        exit(EXIT_FAILURE);
      }
    }

    auto root = recipe->get<std::string>("inside.root", "");
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

    for (auto const& service : recipe->node()["service.system"]) {
      INFO("enabling system service");
      if (Executor::execute("systemctl enable " + service.as<std::string>()) !=
          0) {
        exit(EXIT_FAILURE);
      }
    }

    for (auto const& service : recipe->node()["service.user"]) {
      INFO("enabling user service");
      if (Executor::execute("systemctl enable --global " +
                            service.as<std::string>()) != 0) {
        exit(EXIT_FAILURE);
      }
    }

    std::string inside = recipe->get<std::string>("inside", "");
    if (inside.size()) {
      INFO("executing inside script");
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
