#include "common.hh"

TEST_F(LibPkgupdTest, DependenciesResolveTest) {
  auto depend = pkgupd->depends({"d", "a"});
  ASSERT_EQ(depend.size(), 4);
  ASSERT_EQ(depend[0], "a");
  ASSERT_EQ(depend[1], "b");
  ASSERT_EQ(depend[2], "c");
  ASSERT_EQ(depend[3], "d");
};