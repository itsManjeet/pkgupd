#include "Http.h"

#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <netdb.h>
#include <regex>
#include <sys/socket.h>

#define DOWNLOAD_BUFFER_SIZE 1024

static inline std::string humanize(size_t bytes) {
    if (bytes >= 1073741824) {
        return std::to_string(bytes / 1073741824) + " GiB";
    } else if (bytes >= 1048576) {
        return std::to_string(bytes / 1048576) + " MiB";
    } else if (bytes >= 1024) {
        return std::to_string(bytes / 1024) + " KiB";
    }
    return std::to_string(bytes) + " Bytes";
}

int progress_func(
        const char* filename, ssize_t downloaded, ssize_t total_size) {
    if (total_size <= 0.0) { return 0; }

    int const totaldotz = 40;
    double const fractiondownloaded = (double)downloaded / (double)total_size;
    auto const dotz = (int)round(fractiondownloaded * totaldotz);

    int ii = 0;
    printf("Downloading %s %3.0f%% ", filename, fractiondownloaded * 100);
    printf("\033[1m [%.10s/%.10s]\033[0m                                       "
           "    \r",
            humanize(static_cast<size_t>(downloaded)).c_str(),
            humanize(static_cast<size_t>(total_size)).c_str());
    fflush(stdout);
    return 0;
}

struct SocketDeleter {
    using pointer = int;

    void operator()(int fd) const {
        if (fd != -1) { close(fd); }
    }
};

static std::tuple<std::string, std::string, std::string> parse_url(
        const std::string& url) {
    std::regex pattern("(http|https)://([^/]+)(.*)");
    std::smatch matches;
    if (std::regex_match(url, matches, pattern)) {
        if (matches.size() == 4) {
            return {matches[1], matches[2], matches[3]};
        }
        throw std::runtime_error("invalid url format");
    }
    throw std::runtime_error("url doesn't match the pattern");
}

Http::Http() = default;

Http::~Http() = default;

Http& Http::url(const std::string& url) {
    DEBUG("URL : " << url)
    m_url = url;
    return *this;
}

void Http::download(const std::filesystem::path& filepath) {
    if (filepath.has_parent_path() &&
            !std::filesystem::exists(filepath.parent_path())) {
        std::filesystem::create_directories(filepath.parent_path());
    }

    auto tempfile = filepath.string() + ".tmp";
    std::ofstream writer(tempfile, std::ios_base::binary);

    perform(&writer);
    std::filesystem::rename(tempfile, filepath);
}

void Http::perform(std::ostream* os) {
    auto [protocol, hostname, path] = parse_url(m_url);
    DEBUG("PROTOCOL  : " << protocol);
    DEBUG("HOSTNAME  : " << hostname);
    DEBUG("PATH      : " << path);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    std::shared_ptr<void> _(nullptr, [&socket_fd](...) { close(socket_fd); });
    if (socket_fd < 0) { throw std::runtime_error("failed to create socket"); }

    auto const server = gethostbyname(hostname.c_str());
    if (server == nullptr) {
        throw std::runtime_error("failed to resolve host url " + m_url);
    }

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(protocol == "http" ? 80 : 433);
    memcpy(&server_addr.sin_addr.s_addr, *server->h_addr_list,
            strlen(*server->h_addr_list));

    if (connect(socket_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        throw std::runtime_error("failed to connect to host");
    }

    std::stringstream request;
    request << "GET " << path << " HTTP/1.1\r\n"
            << "Host: " << hostname << "\r\n"
            << "Connection: close\r\n\r\n";

    if (send(socket_fd, request.str().c_str(), request.str().length(), 0) < 0) {
        throw std::runtime_error("failed to send request");
    }

    char buffer[DOWNLOAD_BUFFER_SIZE];
    ssize_t bytes_received;
    std::string response;
    std::string extra_data;
    while ((bytes_received = recv(socket_fd, buffer, 10, 0)) > 0) {
        response.append(buffer, 0, bytes_received);
        if (auto iter = response.find("\r\n\r\n"); iter != std::string::npos) {
            extra_data = response.substr(iter + 4);
            response = response.substr(0, iter);
            break;
        }
    }

    std::map<std::string, std::string> response_map;
    std::stringstream ss(response);
    for (std::string line; std::getline(ss, line);) {
        if (auto idx = line.find_first_of(':'); idx != std::string::npos) {
            response_map[line.substr(0, idx)] = line.substr(idx + 2);
        }
    }

    ssize_t content_length = 0;
    if (auto idx = response_map.find("Content-Length");
            idx != response_map.end()) {
        content_length = std::stol(idx->second);
    }

    DEBUG("CONTENT LENGTH : " << content_length);

    if (!extra_data.empty()) {
        os->write(extra_data.data(), extra_data.length());
    }

    ssize_t total_downloaded = 0;
    while ((bytes_received = recv(socket_fd, buffer, DOWNLOAD_BUFFER_SIZE, 0)) >
            0) {
        os->write(buffer, bytes_received);
        progress_func(std::filesystem::path(m_url).filename().c_str(),
                total_downloaded, content_length);
        total_downloaded += bytes_received;
    }
    if (bytes_received > 0) {
        throw std::runtime_error("error while receiving file");
    }
    std::cout << std::endl;
}

nlohmann::json Http::get() {
    std::stringstream ss;
    perform(&ss);

    return nlohmann::json::parse(ss.str());
}