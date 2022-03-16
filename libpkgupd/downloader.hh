#ifndef _DOWNLOADER_HH_
#define _DOWNLOADER_HH_

#include "defines.hh"

namespace rlxos::libpkgupd {
class Downloader : public Object {
 private:
  std::vector<std::string> m_Mirrors;
  std::string m_Version;

  bool valid(std::string const &url);

  static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *fstream);

 public:
  Downloader(std::vector<std::string> mirrors, std::string version)
      : m_Mirrors(mirrors), m_Version(version) {}

  bool get(std::string const &file, std::string const& repo, std::string const &out);

  bool download(std::string const &url, std::string const &out);
};

}  // namespace rlxos::libpkgupd

#endif