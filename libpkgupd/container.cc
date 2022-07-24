#include "container.hh"

#include <sched.h>
#include <string.h>
#include <unistd.h>

#include <iostream>

using namespace rlxos::libpkgupd;

std::map<std::string, int> const clone_flags{
    {"cgroup", CLONE_NEWCGROUP}, {"ipc", CLONE_NEWIPC}, {"ns", CLONE_NEWNS},
    {"net", CLONE_NEWNET},       {"pid", CLONE_NEWPID}, {"uts", CLONE_NEWUTS},
    {"user", CLONE_NEWUSER},
};

bool Container::run(std::vector<std::string> data, bool debug) {
  int flags = 0;
  std::vector<std::string> flags_id;
  mConfig->get("clone", flags_id);
  for (auto const& i : flags_id) {
    auto iter = clone_flags.find(i);
    if (iter == clone_flags.end()) {
      p_Error = "invalid clone flag '" + i + "'";
      return false;
    }
    DEBUG("cloning new " + i);
    flags |= iter->second;
  }

  std::vector<char const*> args(data.size() + 1);
  for (int i = 0; i < data.size(); ++i) args[i] = data[i].c_str();
  args[args.size() - 1] = nullptr;

  if (unshare(flags) == -1) {
    p_Error = "unshare() failed: " + std::string(strerror(errno));
    return false;
  }

  int status = execvp(args[0], (char* const*)args.data());
  if (status != 0) {
    p_Error = "execvp() failed: " + std::string(strerror(errno));
    return false;
  }

  p_Error = "execvp() exit: " + std::string(strerror(errno));
  return false;
}