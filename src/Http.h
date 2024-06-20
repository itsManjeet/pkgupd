#ifndef PKGUPD_HTTP_H
#define PKGUPD_HTTP_H

#include "Configuration.h"
#include "defines.hxx"
#include "json.h"
#include <utility>

class Http {
private:
    std::string m_url;

    void perform(std::ostream* os);

public:
    Http();
    ~Http();

    Http& url(const std::string& u);

    nlohmann::json get();

    void download(const std::filesystem::path& filepath);
};

#endif
