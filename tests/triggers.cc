#include <gtest/gtest.h>

#include "../libpkgupd/triggerer.hh"

using namespace rlxos;

class TriggerTest : public ::testing::Test {
 protected:
  class PkgupdTriggererTester : public libpkgupd::triggerer {
   public:
    bool triggers_checkup(std::vector<type> const& t,
                          std::vector<std::string> const& s) {
      std::vector<std::vector<std::string>> filelist_;
      filelist_.push_back(s);

      std::vector<type> triggers = _get(filelist_);
      for (auto const& i : triggers) {
        if (std::find(t.begin(), t.end(), i) == t.end()) {
          return false;
        }
      }
      return true;
    }
  };
  PkgupdTriggererTester tester;
};

TEST_F(TriggerTest, RegrexPatternCheck) {
  EXPECT_TRUE(
      tester.triggers_checkup({libpkgupd::triggerer::type::GLIB_SCHEMAS},
                              {"./usr/share/glib-2.0/schemas/org.xfce.mousepad.gschema.xml"}));
}