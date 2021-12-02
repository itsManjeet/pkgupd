#ifndef _DOWNLOADER_HH_
#define _DOWNLOADER_HH_

#include "defines.hh"

namespace rlxos::libpkgupd {
class downloader : public object {
 private:
  std::vector<std::string> _urls;

  bool valid(std::string const &url);

  static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *fstream);

 public:
  downloader() {}

  METHOD(std::vector<std::string>, urls);

  bool get(std::string const &file, std::string const &out);

  bool download(std::string const &url, std::string const &out);
};

}  // namespace rlxos::libpkgupd

#endif