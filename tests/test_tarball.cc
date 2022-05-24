// #include <gtest/gtest.h>

// #include <libpkgupd/archive-manager/tarball/tarball.hh>
// #include <libpkgupd/utils/utils.hh>
// using namespace rlxos;

// #include <fstream>
// using namespace std;

// class TestTarBall : public testing::Test {
//  protected:
//   std::shared_ptr<libpkgupd::ArchiveManager> archive_manager =
//       libpkgupd::ArchiveManager::create(libpkgupd::ArchiveManagerType::TARBALL);
//   char const* tarball = DATA_DIR "/zsync.pkg";
//   std::vector<std::string> req_files_list = {
//       "./",
//       "./info",
//       "./usr/",
//       "./usr/bin/",
//       "./usr/bin/zsyncmake",
//       "./usr/bin/zsync",
//       "./usr/share/",
//       "./usr/share/man/",
//       "./usr/share/man/man1/",
//       "./usr/share/man/man1/zsync.1",
//       "./usr/share/man/man1/zsyncmake.1",
//       "./usr/share/doc/",
//       "./usr/share/doc/zsync/",
//       "./usr/share/doc/zsync/COPYING",
//       "./usr/share/doc/zsync/README",
//   };
// };

// TEST_F(TestTarBall, test_tarball_get) {
//   std::string info_data;
//   auto status = archive_manager->get(tarball, "./info", info_data);
//   ASSERT_TRUE(status);

//   std::string required_content = R"(id: zsync
// version: 0.6.2
// about: A file transfer program that able to connect to rsync servers
// repository: core
// type: pkg
// depends:
//  - glibc
// )";
//   ASSERT_EQ(info_data, required_content);
// }

// TEST_F(TestTarBall, test_tarball_get_fail) {
//   std::string non_existing_file;
//   auto status =
//       archive_manager->get(tarball, "some-missing/file", non_existing_file);
//   ASSERT_FALSE(status);
//   ASSERT_EQ(non_existing_file, "");
// }

// TEST_F(TestTarBall, test_tarball_list) {
//   std::vector<std::string> files_list;
//   auto status = archive_manager->list(tarball, files_list);

//   ASSERT_EQ(files_list.size(), req_files_list.size());
//   for (auto i = 0; i < files_list.size(); i++) {
//     ASSERT_EQ(files_list[i], req_files_list[i]);
//   }
// }

// TEST_F(TestTarBall, test_tarball_extract) {
//   // TODO: fixed tarfiles
//   std::vector<std::string> extracted_files;

//   std::filesystem::remove_all(CACHE_DIR "/new-dir");
//   auto status =
//       archive_manager->extract(tarball, CACHE_DIR "/new-dir", extracted_files);
//   ASSERT_TRUE(status);
//   ASSERT_TRUE(std::filesystem::exists(CACHE_DIR "/new-dir"));

//   std::vector<std::string> files_list;
//   ASSERT_TRUE(archive_manager->list(tarball, files_list));

//   for (auto const& file : files_list) {
//     if (file == "./info") continue;
//     auto filepath = std::string(CACHE_DIR "/new-dir") + "/" + file;
//     ASSERT_TRUE(std::filesystem::exists(filepath)) << filepath << " not exists";
//   }
// }