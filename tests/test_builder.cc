#include <gtest/gtest.h>

#include <libpkgupd/archive-manager/archive-manager.hh>
#include <libpkgupd/builder.hh>
#include <libpkgupd/defines.hh>
#include <libpkgupd/recipe.hh>
using namespace rlxos::libpkgupd;

#include <fstream>
using namespace std;

const string CONFIG = "dir.pkgs: " CACHE_DIR
                      "/pkgs\n"
                      "dir.src: " CACHE_DIR "/src";

const string TEST_PACKAGE = R"(
id: test-package
version: 0.0.1
about: Sample package for testing
script: |
  echo 'int main() {return 0;}' > main.c
  echo 'int add(int a, int b) {return a + b;}' > add.c

  mkdir -p ${pkgupd_pkgdir}/usr/{bin,lib}
  gcc main.c -o ${pkgupd_pkgdir}/usr/bin/test
  gcc -fPIC -shared add.c -o ${pkgupd_pkgdir}/usr/lib/libadd.so

split:
  - into: libadd
    about: test-package runtime
    files:
      - usr/lib/libadd.so)";

class TestBuilder : public testing::Test {
 protected:
  shared_ptr<Configuration> config;
  shared_ptr<Builder> builder;

  void init() {
    config = make_shared<Configuration>(YAML::Load(CONFIG));
    builder = make_shared<Builder>(config.get());
  }

//   void cleanup() { std::filesystem::remove_all(CACHE_DIR); }
};

TEST_F(TestBuilder, TestPackageBuild) {
  init();
//   DEFER(cleanup(););

  auto node = YAML::Load(TEST_PACKAGE);
  shared_ptr<Recipe> recipe =
      make_shared<Recipe>(node, "test-package", "testing");

  ASSERT_TRUE(builder->build(recipe.get()));

  for (auto const& pkg_id : {"test-package", "libadd"}) {
    auto i = recipe->operator[](pkg_id);
    auto pkgpath =
        filesystem::path(config->get<string>(DIR_PKGS, DEFAULT_PKGS_DIR)) /
        i->repository() / (PACKAGE_FILE(i.get()));
    ASSERT_TRUE(filesystem::exists(pkgpath));
    std::shared_ptr<ArchiveManager> archive_manager =
        ArchiveManager::create(i->type());
    ASSERT_TRUE(archive_manager != nullptr);
    vector<string> extracted_files;
    ASSERT_TRUE(archive_manager->extract(pkgpath.c_str(), CACHE_DIR "/roots",
                                         extracted_files));
    if (pkg_id == "test-package") {
      ASSERT_EQ(Executor::execute("file usr/bin/test | grep -q 'not stripped'",
                                  CACHE_DIR "/roots"),
                1);
    }
  }
}