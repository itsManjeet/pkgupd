#ifndef _LIBPKGUPD_DEFINES_HH_
#define _LIBPKGUPD_DEFINES_HH_

#include <assert.h>

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "colors.hh"

namespace rlxos::libpkgupd {
class object {
 protected:
  std::string _error;

 public:
  std::string const &error() const { return _error; }
};

}  // namespace rlxos::libpkgupd

#define GET_METHOD(type, var) \
  type const &var() const { return _##var; }

#define SET_METHOD(type, val) \
  void val(type val) { _##val = val; }

#define METHOD(type, var) \
  GET_METHOD(type, var)   \
  SET_METHOD(type, var)

#define _CHECK_VALUE(type, variable) \
  if (data[#variable]) _##variable = data[#variable].as<type>();

#define _CHECK_LIST(type, variable) \
  if (data[#variable])              \
    for (auto const &i : data[#variable]) _##variable.push_back(i.as<type>());

#define _THROW_ERROR(variable) \
  else throw std::runtime_error(#variable " is missing in " + file);

#define _USE_FALLBACK(variable, fallback) else _##variable = fallback;

#define READ_VALUE(type, variable) \
  _CHECK_VALUE(type, variable)     \
  _THROW_ERROR(variable)

#define READ_LIST(type, variable) _CHECK_LIST(type, variable)

#define READ_OBJECT_LIST(type, variable)  \
  if (data[#variable])                    \
    for (auto const &i : data[#variable]) \
      _##variable.push_back(std::make_shared<type>(i, file));

#define READ_LIST_FROM(type, variable, from, into) \
  if (data[#from] && data[#from][#variable])       \
    for (auto const &i : data[#from][#variable])   \
      _##into.push_back(i.as<type>());

#define OPTIONAL_VALUE(type, variable, fallback) \
  _CHECK_VALUE(type, variable)                   \
  _USE_FALLBACK(variable, fallback)

#define DEFAULT_DATA_DIR "/var/lib/pkgupd/data"
#define DEFAULT_PKGS_DIR "/var/cache/pkgupd/pkgs"
#define DEFAULT_SRC_DIR "/var/cache/pkgupd/src"
#define DEFAULT_REPO_DIR "/var/cache/pkgupd/recipes"
#define DEFAULT_ROOT_DIR "/"
#define DEFAULT_URL "https://rlxos.cloudtb.online/pkgs"
#define DEFAULT_SECONDARY_URL "https://apps.rlxos.dev/pkgs"

#define DEFAULT_ARCHIVE_TOOL "tar"
#define BUG_URL "https://rlxos.dev/bugs"

#define DEFAULT_EXTENSION "rlx"

#define MAX_STRING_SIZE 512
#endif