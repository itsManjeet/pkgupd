#include <gtest/gtest.h>

#include <libpkgupd/recipe.hh>
using namespace rlxos::libpkgupd;

#include <fstream>
using namespace std;

const string RECIPE_FILE = R"(
id: test
version: 2.0.5
release: 5
about: "{{id }} {{ version}} {{release}} package"
sources:
 - https://url.com/{{id}}-{{version}}.tar.gz
)";

class TestRecipe : public testing::Test {
 protected:
  shared_ptr<Recipe> recipe() {
    YAML::Node node = YAML::Load(RECIPE_FILE);
    return std::make_shared<Recipe>(node, "", "testing");
  }
};

TEST_F(TestRecipe, CheckVariableResolving) {
  std::shared_ptr<Recipe> r = recipe();

  ASSERT_EQ(r->about(), "test 2.0.5 5 package");
  ASSERT_EQ(r->sources()[0], "https://url.com/test-2.0.5.tar.gz");
}