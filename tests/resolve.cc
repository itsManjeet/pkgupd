#include "common.hh"

TEST_F(LibPkgupdTest, DependenciesResolveTest) {
  auto [depend, status] = pkgupd->depends({"d", "a"}, false);
  ASSERT_TRUE(status);
  ASSERT_EQ(depend.size(), 4);
  ASSERT_EQ(depend[0], "a");
  ASSERT_EQ(depend[1], "b");
  ASSERT_EQ(depend[2], "c");
  ASSERT_EQ(depend[3], "d");
};

TEST_F(LibPkgupdTest, DependenciesResolveTest_BuildTime) {
  auto [depend, status] = pkgupd->depends({"d"}, true);
  ASSERT_TRUE(status);
  ASSERT_EQ(depend.size(), 5);
  ASSERT_EQ(depend[0], "a");
  ASSERT_EQ(depend[1], "e");
  ASSERT_EQ(depend[2], "b");
  ASSERT_EQ(depend[3], "c");
  ASSERT_EQ(depend[4], "d");
};