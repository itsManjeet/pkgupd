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
    };

   protected:
    std::string _pkgfile;
    std::shared_ptr<archive::package> _package;

   public:
    archive(std::string const &packagefile)
        : _pkgfile{packagefile} {
    }

    /**
         * @brief Provides the file data of specified file in the package
         * @param filepath path to the file in package (must be started from ./)
         * @return content of file
         */
    virtual std::tuple<int, std::string> getdata(std::string const &filepath) = 0;

    virtual std::shared_ptr<archive::package> info() = 0;
    /**
         * List all files in the archive
         */
    virtual std::vector<std::string> list() = 0;

    virtual bool is_exist(std::string const &path) = 0;

    virtual bool extract(std::string const &outdir) = 0;

    virtual bool compress(std::string const &srcdir, std::shared_ptr<pkginfo> const &info) = 0;
};
}  // namespace rlxos::libpkgupd

#endif