#include "Downloader.h"

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cmath>
#include <regex>

#include <cstring>
#include <filesystem>
#include <iostream>
#include <sys/stat.h>

using std::string;

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

int progress_func(ssize_t downloaded, ssize_t total_size) {
    // ensure that the file to be downloaded is not empty
    // because that would cause a division by zero error later on
    if (total_size <= 0.0) {
        return 0;
    }

    // how wide you want the progress meter to be
    int const totaldotz = 40;
    double const fractiondownloaded = (double) downloaded / (double) total_size;
    // part of the progressmeter that's already "full"
    auto const dotz = (int) round(fractiondownloaded * totaldotz);

    // create the "meter"
    int ii = 0;
    printf("%3.0f%% ", fractiondownloaded * 100);
    // part  that's full already
    for (; ii < dotz; ii++) {
        printf("\033[32;1m▬\033[0m");
    }
    // remaining part (spaces)
    for (; ii < totaldotz; ii++) {
        printf("\033[1m▬\033[0m");
    }
    // and back to line begin - do not forget the fflush to avoid output buffering
    // problems!
    printf("\033[1m [%.10s/%.10s]\033[0m                                           \r",
           humanize(static_cast<size_t>(downloaded)).c_str(),
           humanize(static_cast<size_t>(total_size)).c_str());
    fflush(stdout);
    // if you don't return 0, the transfer will be aborted - see the documentation
    return 0;
}

struct SocketDeleter {
    using pointer = int;

    void operator()(int fd) const {
        if (fd != -1) {
            close(fd);
        }
    }
};

static std::tuple<std::string, std::string, std::string> parse_url(std::string url) {
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

void Downloader::download(const std::string &url, const std::filesystem::path &outfile) {
    if (outfile.has_parent_path() && !std::filesystem::exists(outfile.parent_path())) {
        std::filesystem::create_directories(outfile.parent_path());
    }
    auto [protocol, hostname, path] = parse_url(url);
    DEBUG("PROTOCOL  : " << protocol);
    DEBUG("HOSTNAME  : " << hostname);
    DEBUG("PATH      : " << path);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    std::shared_ptr<void> _(nullptr, [&socket_fd](...) {
        close(socket_fd);
    });
    if (socket_fd < 0) {
        throw std::runtime_error("failed to create socket");
    }

    auto const server = gethostbyname(hostname.c_str());
    if (server == nullptr) {
        throw std::runtime_error("failed to resolve host url " + url);
    }

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(protocol == "http" ? 80 : 433);
    memcpy(&server_addr.sin_addr.s_addr, *server->h_addr_list, strlen(*server->h_addr_list));

    if (connect(socket_fd, (sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        throw std::runtime_error("failed to connect to host");
    }

    std::stringstream request;
    request << "GET " << path << " HTTP/1.1\r\n"
            << "Host: " << hostname << "\r\n"
            << "Connection: close\r\n\r\n";

    if (send(socket_fd, request.str().c_str(), request.str().length(), 0) < 0) {
        throw std::runtime_error("failed to send request");
    }

    std::unique_ptr<FILE, decltype(&fclose)> output_file(fopen((outfile.string() + ".tmp").c_str(), "wb"), fclose);
    if (output_file.get() == nullptr) {
        throw std::runtime_error("failed to open output file for write " + outfile.string());
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
    if (auto idx = response_map.find("Content-Length"); idx != response_map.end()) {
        content_length = std::stol(idx->second);
    }

    DEBUG("CONTENT LENGTH : " << content_length);

    if (!extra_data.empty()) {
        fwrite(extra_data.data(), 1, extra_data.length(), output_file.get());
    }
    ssize_t total_downloaded = 0;
    while ((bytes_received = recv(socket_fd, buffer, DOWNLOAD_BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes_received, output_file.get());
        progress_func(total_downloaded, content_length);
        total_downloaded += bytes_received;
    }
    if (bytes_received > 0) {
        throw std::runtime_error("error while receiving file");
    }
    std::cout << std::endl;


    std::filesystem::rename(outfile.string() + ".tmp", outfile);
}

bool Downloader::valid(char const *url) {
    return true;
}