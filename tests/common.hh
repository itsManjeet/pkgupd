#ifndef LIBPKGUPD_TEST_COMMON
#define LIBPKGUPD_TEST_COMMON

#include <gtest/gtest.h>

#include "../libpkgupd/libpkgupd.hh"
using namespace rlxos;
using std::string;

class LibPkgupdTest : public testing::Test {
 protected:
  std::shared_ptr<libpkgupd::Pkgupd> pkgupd;
  LibPkgupdTest() {
    pkgupd = std::make_shared<libpkgupd::Pkgupd>(
        DATA_DIR "/system", DATA_DIR "/cache",
        std::vector<std::string>{"https://apps.rlxos.dev"},
        std::vector<std::string>{"core"}, "2200", DATA_DIR "/roots", false,
        false);
  }
};

#endif