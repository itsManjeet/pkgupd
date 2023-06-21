#ifndef LIBPKGUPD_BUNDLER_HH
#define LIBPKGUPD_BUNDLER_HH

#include "configuration.hh"
#include "recipe.hh"

namespace rlxos::libpkgupd {
/**
 * @brief Bundler helps to bundle all the required files and libraries
 * needed to generate a self contained bundled
 *
 */
class Bundler : public Object {
 private:
  std::string m_WorkDir;  //!< Holds the path of working directory which holds
                          //!< the bundled files
  std::string m_RootDir;  //<! Holds the path to roots from where libraries and
                          // files will be taken

 public:
  Bundler(std::string workdir, std::string rootdir)
      : m_WorkDir(workdir), m_RootDir(rootdir) {}
  /**
   * @brief wrapper for ldd to list required libraries
   *
   * @param path
   * @return std::set<std::string>
   */
  std::set<std::string> ldd(std::string path);

  /**
   * @brief add all the libraries required by all the binaries and shared
   * libraries in the workdir
   *
   * @param exclude exclude specified files to be bundled
   * @return true on success
   * @return false on failure with error message set
   */
  bool resolveLibraries(std::vector<std::string> const& exclude);

  /**
   * @brief return the mime type of specified path or throw runtime_error
   *
   * @param path path to file
   * @return std::string mimetype of file
   */
  static std::string mime(std::string path);
};
}  // namespace rlxos::libpkgupd

#endif