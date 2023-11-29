#ifndef _DOWNLOADER_HH_
#define _DOWNLOADER_HH_

#include "configuration.hxx"
#include "defines.hxx"

namespace rlxos::libpkgupd {
    class Downloader {
        Configuration* mConfig;

        bool valid(char const* url);

        static size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* fstream);

    public:
        Downloader(Configuration* config) : mConfig{config} {
        }

        void get(const std::filesystem::path& file, const std::filesystem::path& outdir);

        void download(const std::string& url, const std::filesystem::path& outfile);
    };
} // namespace rlxos::libpkgupd

#endif
