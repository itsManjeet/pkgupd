#include <gtest/gtest.h>

#include "../libpkgupd/utils/utils.hh"
using namespace rlxos::libpkgupd;

TEST(UtilsTest, GetVersionTest) {
    ASSERT_EQ(utils::get_version("102ab.103.104"),102103104);
}