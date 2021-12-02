#ifndef _PKGUPD_HH_
#define _PKGUPD_HH_

#include <yaml-cpp/yaml.h>

#include <map>
#include <string>
#include <vector>

#include "../libpkgupd/libpkgupd.hh"

namespace rlxos::libpkgupd {

class PKGUPD {
 public:
  enum class task : int {
    INVLAID,
    INSTALL,
    COMPILE,
    REMOVE,
    REFRESH,
    SEARCH,
    UPDATE,
    DEPTEST,
    INFO,
    TRIGGERS,
  };

  enum class flag : int {
    FORCE,
    SKIP_TRIGGER,
    SKIP_DEPENDS,
    NO_INSTALL,
    REPOSITORY,
  };

 private:
  task _task;

  std::map<std::string, flag> _aval_flags{
      {"force", flag::FORCE},
      {"skip-triggers", flag::SKIP_TRIGGER},
      {"skip-depends", flag::SKIP_DEPENDS},
      {"no-install", flag::NO_INSTALL},
      {"repository", flag::REPOSITORY},
  };

  std::vector<flag> _flags;
  std::vector<std::string> _args;
  std::map<std::string, std::string> _values;

  YAML::Node _config;

  std::string SYS_DB = "sys-db";
  std::string REPO_DB = "repo-db";
  std::string PKG_DIR = "pkg-dir";
  std::string SRC_DIR = "src-dir";
  std::string ROOT_DIR = "root-dir";

  std::string _config_file = "/etc/pkgupd.yml";

  void _print_help(char const *path);

  void _parse_args(int ac, char **av);

  bool _need_atleast(int size);
  bool _need_args(int size);

  std::string _get_value(std::string var, std::string def);

  bool _is_flag(flag f) {
    return (std::find(_flags.begin(), _flags.end(), f) != _flags.end());
  }

 public:
  int exec(int ac, char **av);
};

}  // namespace rlxos::libpkgupd

#endif