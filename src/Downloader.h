#ifndef _DOWNLOADER_HH_
#define _DOWNLOADER_HH_

#include <utility>
#include "json.h"

#include "Configuration.h"
#include "defines.hxx"


class Downloader {
    const std::string url;
    std::map<std::string, std::string> header;

    void perform(std::ostream &body);

public:
    explicit Downloader(std::string url) : url{std::move(url)} {

    }

    nlohmann::json get_json();

    void save(const std::filesystem::path &outfile);

};


#endif
