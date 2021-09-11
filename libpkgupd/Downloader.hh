#ifndef _DOWNLOADER_HH_
#define _DOWNLOADER_HH_

#include "Defines.hh"

namespace rlxos::libpkgupd
{
    class Downloader : public Object
    {
    private:
        std::vector<std::string> urls;

        bool performCurl(std::string const &url, std::string const &outdir);

        bool isExist(std::string const &url);

        static size_t writeData(void *ptr, size_t size, size_t nmemb, FILE *fstream);

    public:
        Downloader()
        {
        }

        void AddURL(std::string const &u)
        {
            urls.push_back(u);
        }

        bool Download(std::string const &file, std::string const &outdir);
    };

}

#endif