#ifndef _PKGUPD_EXTRACTOR_HH_
#define _PKGUPD_EXTRACTOR_HH_

#include <yaml-cpp/yaml.h>

#include <optional>

#include "../defines.hxx"
#include "../package-info.hxx"

namespace rlxos::libpkgupd {

#define ARCHIVE_TYPE_LIST \
  X(TARBALL, TarBall)     \
  X(SQUASH, Squash)       \
  X(APPIMAGE, AppImage)

/**
 * @brief ArchiveManagerType holds the supported archive manager types
 */
    enum class ArchiveManagerType : uint8_t {
#define X(ID, Object) ID,
        ARCHIVE_TYPE_LIST
#undef X
        N_ARCHIVE_TYPE
    };

#define ARCHIVE_TYPE_INT(id) static_cast<int>(id)

    static const char
            *ARCHIVE_TYPE_STR[ARCHIVE_TYPE_INT(ArchiveManagerType::N_ARCHIVE_TYPE)] = {
#define X(ID, Object) #ID,
            ARCHIVE_TYPE_LIST
#undef X
    };

/**
 * This class represent rlxos compressed Package,
 * @brief It provides various methods to handle, read, compress and extract
 * rlxos packages
 */
    class ArchiveManager : public Object {
    public:
        /**
         * @brief Provides the file data of specified file in the package
         * @param filepath path to the file in Package (must be started from ./)
         * @return content of file
         */
        virtual bool get(char const *archive_file, char const *input_path,
                         std::string &output) = 0;

        virtual bool extract_file(char const *archive_file, char const *input_path,
                                  char const *output_file) = 0;

        virtual std::shared_ptr<PackageInfo> info(char const *) = 0;

        /**
         * List all files in the archive
         */
        virtual bool list(char const *, std::vector<std::string> &) = 0;

        virtual bool extract(char const *, char const *,
                             std::vector<std::string> &) = 0;

        virtual bool compress(char const *, char const *) = 0;

        static std::shared_ptr<ArchiveManager> create(ArchiveManagerType type);

        static std::shared_ptr<ArchiveManager> create(PackageType type);

        static std::shared_ptr<ArchiveManager> create(std::string packagePath);
    };
}  // namespace rlxos::libpkgupd

#endif