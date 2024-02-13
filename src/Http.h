#ifndef _DOWNLOADER_HH_
#define _DOWNLOADER_HH_

#include "Configuration.h"
#include "defines.hxx"
#include "json.h"
#include <curl/curl.h>
#include <utility>

class Http {
private:
    CURL* curl{nullptr};

    void perform(std::ostream* os);

public:
    Http();
    ~Http();

    Http& url(const std::string& u);

    nlohmann::json get();

    void download(const std::filesystem::path& filepath);
};

#endif
