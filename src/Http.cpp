#include "Http.h"

#include <format>
#include <fstream>

Http::Http() {
    curl = curl_easy_init();
    if (!curl) throw std::runtime_error("failed to init curl");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
}

Http::~Http() {
    if (curl) curl_easy_cleanup(curl);
}

Http& Http::url(const std::string& u) {
    curl_easy_setopt(curl, CURLOPT_URL, u.c_str());
    return *this;
}

void Http::download(const std::filesystem::path& filepath) {
    if (filepath.has_parent_path() &&
            !std::filesystem::exists(filepath.parent_path())) {
        std::filesystem::create_directories(filepath.parent_path());
    }

    auto tempfile = filepath.string() + ".tmp";
    std::ofstream writer(tempfile, std::ios_base::binary);

    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    perform(&writer);
    std::filesystem::rename(tempfile, filepath);
}

void Http::perform(std::ostream* os) {
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void*>(os));
    curl_easy_setopt(
            curl, CURLOPT_WRITEFUNCTION,
            +[](void* data, size_t size, size_t nmemb,
                     void* user_data) -> size_t {
                auto os = reinterpret_cast<std::ostream*>(user_data);
                os->write(reinterpret_cast<const char*>(data), size * nmemb);
                return size * nmemb;
            });

    auto res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        throw std::runtime_error(
                std::format("curl::perform() failed {}", (int)res));
    }
}

nlohmann::json Http::get() {
    std::stringstream ss;
    perform(&ss);

    return nlohmann::json::parse(ss.str());
}