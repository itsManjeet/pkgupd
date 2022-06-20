#include "container.hh"

#include <unistd.h>

#include <iostream>

using namespace rlxos::libpkgupd;

bool Container::run(std::vector<std::string> data, bool debug) {
  std::vector<std::string> args;
  std::string exec = mConfig->get<std::string>("bubblewrap", "/bin/bwrap");
  args.insert(args.end(), {exec});
  std::string root_dir = mConfig->get<std::string>("roots", "");
  if (root_dir.length())
    args.insert(args.end(), {"--bind", root_dir, "/", "--lock-file", "/.lock"});
  else {
    args.insert(args.end(), {"--bind", "/", "/"});
  }

  args.insert(args.end(), {"--dev", "/dev", "--proc", "/proc"});

  for (auto const& i : mConfig->node()["binds"]) {
    args.insert(args.end(), {"--bind", i.first.as<std::string>(),
                             i.second.as<std::string>()});
    args.insert(args.end(),
                {"--lock-file", (i.second.as<std::string>() + "/.lock")});
  }

  for (auto const& i : mConfig->node()["ro-binds"])
    args.insert(args.end(), {"--ro-bind", i.first.as<std::string>(),
                             i.second.as<std::string>()});

  for (auto const& sym : mConfig->node()["symlinks"])
    args.insert(args.end(), {"--symlink", sym.first.as<std::string>(),
                             sym.second.as<std::string>()});

  for (auto const& dev : mConfig->node()["devices"])
    args.insert(args.end(), {"--dev-bind", dev.first.as<std::string>(),
                             dev.second.as<std::string>()});

  for (auto const& env : mConfig->node()["environ"])
    args.insert(args.end(), {"--setenv", env.first.as<std::string>(),
                             env.second.as<std::string>()});

  if (mConfig->node()["hostname"])
    args.insert(args.end(), {"--hostname",
                             mConfig->get<std::string>("hostname", "subsystem"),
                             "--unshare-uts"});

  if (mConfig->node()["work-dir"]) {
    args.insert(args.end(),
                {"--chdir", mConfig->get<std::string>("work-dir", "/")});
  }

  for (auto const& i : mConfig->node()["bwrap"]) {
    args.push_back(i.as<std::string>());
  }

  for (auto const& un : mConfig->node()["unshare"]) {
    std::string c = un.as<std::string>();
    if (c == "pid") {
      args.push_back("--unshare-pid");
    } else if (c == "all") {
      args.push_back("--unshare-all");
    } else if (c == "net") {
      args.push_back("--unshare-net");
    } else if (c == "user") {
      args.push_back("--unshare-user");
    } else if (c == "user-try") {
      args.push_back("--unshare-user-try");
    } else if (c == "ipc") {
      args.push_back("--unshare-ipc");
    } else if (c == "uts") {
      args.push_back("--unshare-uts");
    } else if (c == "cgroup") {
      args.push_back("--unshare-cgroup");
    } else if (c == "cgroup-try") {
      args.push_back("--unshare-cgroup-try");
    }
  }
  for (auto const& i : data) args.push_back(i.data());

  if (debug) {
    std::string sep;
    for (auto const& i : args) {
      std::cout << sep << i;
      sep = " ";
    }
  }

  std::vector<char const*> cargs;
  cargs.reserve(args.size() + 1);
  for (auto const& i : args) cargs.push_back(i.data());
  cargs.push_back(nullptr);

  int status = execvp(cargs[0], (char**)cargs.data());
  if (status == -1) {
    std::cerr << ":: process terminated unsuccessfully" << std::endl;
    return false;
  }

  return false;
}