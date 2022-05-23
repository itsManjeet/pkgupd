#include <gtest/gtest.h>

#include <libpkgupd/stripper.hh>
using namespace rlxos::libpkgupd;

#include <fstream>
using namespace std;

#define WORK_DIR CACHE_DIR "/stripper"

class TestStripper : public testing::Test {
 protected:
  shared_ptr<Stripper> stripper = std::make_shared<Stripper>();
  void write(string content, string file) {
    ofstream f(file);
    f << content;
  }

  virtual void TearDown() { remove(WORK_DIR); }

  bool is_stripped(string file) {
    return WEXITSTATUS(
        system(("file " + file + " | grep -q 'not stripped'").c_str()));
  }
};

TEST_F(TestStripper, StripBinaryFile) {
  filesystem::create_directories(WORK_DIR);

  write("int main() { return 0; }", WORK_DIR "/bin.c");
  ASSERT_EQ(
      WEXITSTATUS(system("gcc " WORK_DIR "/bin.c -o " WORK_DIR "/bin.bin")), 0);

  stripper->strip(WORK_DIR);
  ASSERT_TRUE(is_stripped(WORK_DIR "/bin.bin"));
};
