#include "common.h"
#include "../libpkgupd/Command.hh"

TEST(Command, GetOutput_Success)
{
    auto cmd = rlxos::libpkgupd::Command("echo", {"5"});
    auto [status, output] = cmd.GetOutput();
    ASSERT_EQ(status, 0);
    ASSERT_EQ(output, "5");
}

TEST(Command, GetOutput_BinaryMissing)
{
    auto cmd = rlxos::libpkgupd::Command("some_missing_binary", {});
    auto [status, output] = cmd.GetOutput();
    ASSERT_NE(status, 0);
}