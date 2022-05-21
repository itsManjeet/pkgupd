#ifndef _LIBPKGUPD_DEFINES_HH_
#define _LIBPKGUPD_DEFINES_HH_

#include <assert.h>
#include <unistd.h>

#include <ctime>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "colors.hh"

namespace rlxos::libpkgupd {
class Object {
 protected:
  std::string p_Error;

 public:
  std::string const &error() const { return p_Error; }
};

static inline std::string generateRandom(int const len) {
  static const char alnum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";

  std::string res;
  for (auto i = 0; i < len; i++) {
    res += alnum[rand() % (sizeof(alnum) - 1)];
  }
  return res;
}

static inline std::string humanize(size_t bytes) {
  if (bytes >= 1073741824) {
    return std::to_string(bytes / 1073741824) + " GiB";
  } else if (bytes >= 1048576) {
    return std::to_string(bytes / 1048576) + " MiB";
  } else if (bytes >= 1024) {
    return std::to_string(bytes / 1024) + " KiB";
  }
  return std::to_string(bytes) + " Bytes";
}

}  // namespace rlxos::libpkgupd

#define GET_METHOD(type, var) \
  type const &var() const { return _##var; }

#define SET_METHOD(type, val) \
  void val(type val) { _##val = val; }

#define METHOD(type, var) \
  GET_METHOD(type, var)   \
  SET_METHOD(type, var)

#define _CHECK_VALUE(type, variableID, variable) \
  if (data[variableID]) variable = data[variableID].as<type>();

#define _CHECK_LIST(type, variableID, variable) \
  if (data[variableID])                         \
    for (auto const &i : data[variableID]) variable.push_back(i.as<type>());

#define _THROW_ERROR(variableID)                               \
  else throw std::runtime_error(variableID " is missing in " + \
                                std::string(file));

#define _USE_FALLBACK(variableID, variable, fallback) else variable = fallback;

#define READ_VALUE(type, variableID, variable) \
  _CHECK_VALUE(type, variableID, variable)     \
  _THROW_ERROR(variableID)

#define READ_LIST(type, variableID, variable) \
  _CHECK_LIST(type, variableID, variable)

#define READ_OBJECT_LIST(type, variableID, variable) \
  if (data[variableID])                              \
    for (auto const &i : data[variableID]) variable.push_back(type(i, file));

#define READ_LIST_FROM(type, variable, from, into) \
  if (data[#from] && data[#from][#variable])       \
    for (auto const &i : data[#from][#variable]) into.push_back(i.as<type>());

#define OPTIONAL_VALUE(type, variableID, variable, fallback) \
  _CHECK_VALUE(type, variableID, variable)                   \
  _USE_FALLBACK(variableID, variable, fallback)

#define DIR_ROOT "dir.root"
#define DIR_CACHE "dir.cache"
#define DIR_DATA "dir.data"
#define DIR_PKGS "dir.pkgs"
#define DIR_SRC "dir.src"
#define DIR_REPO "dir.repo"
#define REPOS "repos"
#define SKIP_TRIGGERS "triggers.skip"

#define DEFAULT_ROOT_DIR "/"
#define DEFAULT_CACHE_DIR "/var/cache/pkgupd"
#define DEFAULT_PKGS_DIR DEFAULT_CACHE_DIR "/pkgs"
#define DEFAULT_SRC_DIR DEFAULT_CACHE_DIR "/src"
#define DEFAULT_REPO_DIR DEFAULT_CACHE_DIR "/repo"
#define DEFAULT_DATA_DIR "/var/lib/pkgupd/data"

#define BUILD_CONFIG_PREFIX "build.config.prefix"
#define BUILD_CONFIG_SYSCONFDIR "build.config.sysconfdir"
#define BUILD_CONFIG_LIBDIR "build.config.libdir"
#define BUILD_CONFIG_LIBEXECDIR "build.config.libexecdir"
#define BUILD_CONFIG_BINDIR "build.config.bindir"
#define BUILD_CONFIG_SBINDIR "build.config.sbindir"
#define BUILD_CONFIG_DATADIR "build.config.datadir"
#define BUILD_CONFIG_LOCALSTATEDIR "build.config.localstatedir"

#define DEFAULT_PREFIX "/usr"
#define DEFAULT_SYSCONFDIR "/etc"
#define DEFAULT_LIBDIR DEFAULT_PREFIX "/lib"
#define DEFAULT_LIBEXECDIR DEFAULT_PREFIX "/lib"
#define DEFAULT_BINDIR DEFAULT_PREFIX "/bin"
#define DEFAULT_SBINDIR DEFAULT_PREFIX "/bin"
#define DEFAULT_DATADIR DEFAULT_PREFIX "/share"
#define DEFAULT_LOCALSTATEDIR "/var"

#define PKGUPD_MODULE(id)                                          \
  extern "C" int PKGUPD_##id(std::vector<std::string> const &args, \
                             rlxos::libpkgupd::Configuration *config)

#define PKGUPD_MODULE_HELP(id) \
  extern "C" void PKGUPD_help_##id(std::ostream &os)
#define CHECK_ARGS(s)                                     \
  if (args.size() != s) {                                 \
    cerr << "need exactly " << s << " arguments" << endl; \
    return 1;                                             \
  }
#endif