#ifndef _PKGUPD_EXTRACTOR_HH_
#define _PKGUPD_EXTRACTOR_HH_

#include <yaml-cpp/yaml.h>

#include "defines.hh"
#include "pkginfo.hh"

namespace rlxos::libpkgupd {
/**
     * This class represent rlxos compressed package,
     * @brief It provides various methods to handle, read, compress and extract rlxos packages
     */
class archive : public object {
   public:
    class package : public pkginfo {
       private:
        std::string _id, _version,
            _about;

        std::string _script;

        std::vector<std::string> _depends;

       public:
        package(YAML::Node const &data, std::string const &file);

        std::string id() const { return _id; }
        std::string version() const { return _version; }
        std::string about() const { return _about; }
        std::vector<std::string> depends(bool) const { return _depends; }

        std::string const &script() const { return _script; }
    };

   private:
    std::string _pkgfile;
    std::string _archive_tool;
    std::shared_ptr<archive::package> _package;

   public:
    archive(std::string const &packagefile,
            std::string const &archivetool = DEFAULT_ARCHIVE_TOOL)
        : _pkgfile{packagefile},
          _archive_tool{archivetool} {
    }

    /**
         * @brief Provides the file data of specified file in the package
         * @param filepath path to the file in package (must be started from ./)
         * @return content of file
         */
    std::tuple<int, std::string> getdata(std::string const &filepath);

    std::shared_ptr<archive::package> info();
    /**
         * List all files in the archive
         */
    std::vector<std::string> list();

    bool is_exist(std::string const &path) const;

    bool extract(std::string const &outdir);

    bool compress(std::string const &srcdir, std::shared_ptr<pkginfo> const &info);
};
}  // namespace rlxos::libpkgupd

#endif