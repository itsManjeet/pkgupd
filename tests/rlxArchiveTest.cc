#include "common.h"
#include "../libpkgupd/rlxArchive.hh"

class rlxArchiveTest : public testing::Test
{
protected:
    std::shared_ptr<rlxos::libpkgupd::rlxArchive> archiver;
    virtual void SetUp()
    {
        archiver = std::make_shared<rlxos::libpkgupd::rlxArchive>(TEST_DATA_PATH "/test-package-0.1.0.rlx");
    }
};

TEST_F(rlxArchiveTest, GetInfo)
{
    auto data = archiver->GetInfo();
    ASSERT_TRUE(data != nullptr);
    ASSERT_TRUE(data->ID() == "test-package");
    ASSERT_TRUE(data->Version() == "0.1.0");
}