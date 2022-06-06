#ifndef LIBPKGUPD_ARCHIVE_MANAGER_SQUASH_HH
#define LIBPKGUPD_ARCHIVE_MANAGER_SQUASH_HH

#include "../archive-manager.hh"

namespace rlxos::libpkgupd {
class Squash : public ArchiveManager {
 protected:
  std::string mOffset = "0";

 public:
  void offset(char const* offset) { mOffset = offset; }

  bool get(char const* app_image, char const* input_path, std::string& output);

  bool extract_file(char const* imagefile, char const* input_path,
                    char const* output_path);

  std::shared_ptr<PackageInfo> info(char const* input_path);

  bool list(char const* input_path, std::vector<std::string>& files);

  bool extract(char const* input_path, char const* output_path,
               std::vector<std::string>&);

  bool compress(char const* input_path, char const* src_dir);
};
}  // namespace rlxos::libpkgupd
#endif