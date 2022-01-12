#include "../libpkgupd/recipe.hh"

#include <gtest/gtest.h>

using namespace rlxos;
using std::string;

class RecipeTest : public testing::Test {
 protected:
  string recipeFilePath = DATA_DIR "/recipe.yml";
};

TEST_F(RecipeTest, ParsingTest) {
  auto node = YAML::LoadFile(recipeFilePath);
  auto recipe = libpkgupd::Recipe(node, recipeFilePath);
}