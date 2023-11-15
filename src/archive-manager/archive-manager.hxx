#ifndef _PKGUPD_EXTRACTOR_HH_
#define _PKGUPD_EXTRACTOR_HH_

#include <yaml-cpp/yaml.h>

#include <optional>

#include "../defines.hxx"
#include "../package-info.hxx"

namespace rlxos::libpkgupd {

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
        bool get(char const *archive_file, char const *input_path,
                 std::string &output);

        bool extract_file(char const *archive_file, char const *input_path,
                          char const *output_file);

        std::shared_ptr<PackageInfo> info(char const *);

        /**
         * List all files in the archive
         */
        bool list(char const *, std::vector<std::string> &);

        bool extract(char const *, char const *,
                     std::vector<std::string> &);

        bool compress(char const *, char const *);
    };
}  // namespace rlxos::libpkgupd

#endif