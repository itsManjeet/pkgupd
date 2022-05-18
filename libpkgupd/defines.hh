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

#define _THROW_ERROR(variableID) \
  else throw std::runtime_error(variableID " is missing in " + file);

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

#define DEFAULT_DATA_DIR "/var/lib/pkgupd/data"
#define DEFAULT_PKGS_DIR "/var/cache/pkgupd/pkgs"
#define DEFAULT_SRC_DIR "/var/cache/pkgupd/src"
#define DEFAULT_ROOT_DIR "/"
#define DEFAULT_URL "https://rlxos.cloudtb.online/pkgs"
#define DEFAULT_SECONDARY_URL "https://apps.rlxos.dev/pkgs"
#define DEFAULT_REPO_DIR "/var/cache/pkgupd/repo"
#define DEFAULT_ARCHIVE_TOOL "tar"
#define BUG_URL "https://rlxos.dev/bugs"

#define DEFAULT_EXTENSION "rlx"

#define MAX_STRING_SIZE 512

#define DEFAULT_CACHE_DIR "/var/cache/pkgupd"

#define PKGUPD_MODULE(id)                                          \
  extern "C" int PKGUPD_##id(std::vector<std::string> const &args, \
                             Configuration *config)
#endif