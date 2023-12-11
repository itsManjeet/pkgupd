#ifndef _DOWNLOADER_HH_
#define _DOWNLOADER_HH_

#include "Configuration.h"
#include "defines.hxx"


class Downloader {
    static bool valid(char const *url);

public:
    static void download(const std::string &url, const std::filesystem::path &outfile);
};


#endif
