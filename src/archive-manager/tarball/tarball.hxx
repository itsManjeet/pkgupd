#ifndef LIBPKGUPD_ARCHIVE_MANAGER_TARBALL_HH
#define LIBPKGUPD_ARCHIVE_MANAGER_TARBALL_HH

#include "../archive-manager.hxx"

namespace rlxos::libpkgupd {
    class TarBall : public ArchiveManager {
    public:
        bool get(char const *tarfile, char const *input_path, std::string &output);

        bool extract_file(char const *tarfile, char const *input_path, char const *output_path);

        std::shared_ptr<PackageInfo> info(char const *input_path);

        bool list(char const *input_path, std::vector<std::string> &files);

        bool extract(char const *input_path, char const *output_path,
                     std::vector<std::string> &);

        bool compress(char const *input_path, char const *src_dir);
    };
}  // namespace rlxos::libpkgupd
#endif