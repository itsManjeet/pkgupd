#include "downloader.hh"

#include <math.h>

#include <filesystem>
#include <iostream>
#include <system_error>

using std::string;
namespace rlxos::libpkgupd {

int progress_func(void *ptr, double TotalToDownload, double NowDownloaded,
                  double TotalToUpload, double NowUploaded) {
    // ensure that the file to be downloaded is not empty
    // because that would cause a division by zero error later on
    if (TotalToDownload <= 0.0) {
        return 0;
    }

    // how wide you want the progress meter to be
    int totaldotz = 40;
    double fractiondownloaded = NowDownloaded / TotalToDownload;
    // part of the progressmeter that's already "full"
    int dotz = (int)round(fractiondownloaded * totaldotz);

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
    printf("\033[1m [%.10s]\033[0m\r",
           humanize(static_cast<size_t>(TotalToDownload)).c_str());
    fflush(stdout);
    // if you don't return 0, the transfer will be aborted - see the documentation
    return 0;
}

bool Downloader::download(char const *url, char const *outfile) {
    std::stringstream ss;
    auto backend = mConfig->get<std::string>("downloader.backend", "curl");
    auto backend_output_flag = (backend == "curl") ? "-o" : "-O";
    ss << backend << " " << mConfig->get<std::string>("downloader.flags", "") << " " << url << " " << backend_output_flag << " " << outfile;
    int status = WEXITSTATUS(system(ss.str().c_str()));
    return status == 0;
}

bool Downloader::valid(char const *url) {
    return true;
}

bool Downloader::get(char const *file, char const *outdir) {
    std::vector<std::string> mirrors;

    mConfig->get("mirrors", mirrors);
    if (mirrors.size() == 0) {
        p_Error = "No mirror specified";
        return false;
    }

    std::string version_part = "/" + mConfig->get<std::string>("version", "2200");

    for (auto const &mirror : mirrors) {
        std::string fileurl = mirror + version_part + "/pkgs/" + file;

        if (mConfig->get("downloader.check", true)) {
            if (!valid(fileurl.c_str())) {
                DEBUG("VERIFICATION FAILED " << p_Error);
                continue;
            }
        }

        return download(fileurl.c_str(), outdir);
    }
    p_Error = string(file) + " is missing on server";

    return false;
}
}  // namespace rlxos::libpkgupd