#include "container.hxx"

#include <sched.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>

using namespace rlxos::libpkgupd;

#define PROC_SETGROUPS "/proc/self/setgroups"
#define PROC_UIDMAP "/proc/self/uid_map"
#define PROC_GIDMAP "/proc/self/gid_map"

std::map<std::string, int> const clone_flags{
        {"cgroup", CLONE_NEWCGROUP},
        {"ipc",    CLONE_NEWIPC},
        {"ns",     CLONE_NEWNS},
        {"net",    CLONE_NEWNET},
        {"pid",    CLONE_NEWPID},
        {"uts",    CLONE_NEWUTS},
        {"user",   CLONE_NEWUSER},
};

bool Container::run(std::vector<std::string> data, bool debug) {
    int flags = 0;
    std::vector<std::string> flags_id;
    mConfig->get("clone", flags_id);
    for (auto const &i: flags_id) {
        auto iter = clone_flags.find(i);
        if (iter == clone_flags.end()) {
            p_Error = "invalid clone flag '" + i + "'";
            return false;
        }
        DEBUG("CLONING " + i);
        flags |= iter->second;
    }

    std::vector<char const *> args(data.size() + 1);
    for (int i = 0; i < data.size(); ++i) args[i] = data[i].c_str();
    args[args.size() - 1] = nullptr;

    if (unshare(flags) == -1) {
        p_Error = "unshare() failed: " + std::string(strerror(errno));
        return false;
    }

    int child_pid;
    pid_t pid = fork();
    if (pid < 0) {
        p_Error = "fork() failed: " + std::string(strerror(errno));
        return false;
    }
    if (pid > 0) {
        int status;
        waitpid(-1, &status, 0);
        if (status != 0) {
            p_Error = "exit code: " + std::to_string(status);
            return false;
        }
        return true;
    }

    DEBUG("PID: " << getpid());
    std::string roots = mConfig->get<std::string>("run.inside", "/");
    if (mount("none", "/", nullptr, MS_PRIVATE | MS_REC, 0) == -1) {
        ERROR("mount(root):none failed: " << strerror(errno));
        exit(1);
    }

    std::vector<std::string> mounts;
    mConfig->get("run.mounts", mounts);
    for (auto const &m: mounts) {
        std::string source = m;
        std::string dest = roots + "/" + m;
        size_t idx = m.find_first_of(':');
        if (idx != std::string::npos) {
            source = m.substr(0, idx);
            dest = roots + "/" + m.substr(idx + 1);
        }
        DEBUG("MOUNTING " << source << " -> " << dest);
        if (mount(source.c_str(), dest.c_str(), nullptr, MS_BIND, 0) == -1) {
            ERROR("mount(" + m + ") failed: " << strerror(errno));
            exit(1);
        }
    }

    if (roots != "/") {
        DEBUG("SWITCHING ROOTS " << roots);
        if (chroot(roots.c_str()) == -1) {
            ERROR("chroot(" + roots + ") failed: " << strerror(errno));
            exit(1);
        }

        auto work_dir = mConfig->get<std::string>("run.dir", "/");
        DEBUG("SWITCHING DIRECTORY " << work_dir);
        if (chdir(work_dir.c_str()) == -1) {
            ERROR("chdir(" + work_dir + ") failed: " << strerror(errno));
            exit(1);
        }
    }

    if (mConfig->get("run.proc", false)) {
        if (roots == "/") {
            if (mount("none", (roots + "/proc").c_str(), nullptr, MS_PRIVATE | MS_REC,
                      nullptr)) {
                ERROR("mount(proc):none failed: " << strerror(errno));
                exit(1);
            }
        }

        DEBUG("MOUNTING PROC");
        if (mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV,
                  nullptr) == -1) {
            ERROR("mount(proc) failed: " << strerror(errno));
            exit(1);
        }
    }

    std::vector<std::string> environments;
    mConfig->get("environ", environments);
    for (auto const &i: environments) {
        putenv((char *) i.c_str());
    }

    execvp(args[0], (char *const *) args.data());
    exit(1);
}