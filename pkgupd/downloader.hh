#ifndef _DOWNLOADER_HH_
#define _DOWNLOADER_HH_

#include "configuration.hh"
#include "defines.hh"

namespace rlxos::libpkgupd {
class Downloader : public Object {
   private:
    Configuration *mConfig;

    bool valid(char const *url);

    static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *fstream);

   public:
    Downloader(Configuration *config) : mConfig{config} {}

    bool get(char const *file, char const *output);

    bool download(char const *url, char const *output);
};

}  // namespace rlxos::libpkgupd

#endif