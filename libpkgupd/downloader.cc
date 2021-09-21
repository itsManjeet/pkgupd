#include "downloader.hh"
#include <iostream>
#include <curl/curl.h>

#include <math.h>

namespace rlxos::libpkgupd
{

    int progress_func(void *ptr, double TotalToDownload, double NowDownloaded,
                      double TotalToUpload, double NowUploaded)
    {
        // ensure that the file to be downloaded is not empty
        // because that would cause a division by zero error later on
        if (TotalToDownload <= 0.0)
        {
            return 0;
        }

        // how wide you want the progress meter to be
        int totaldotz = 40;
        double fractiondownloaded = NowDownloaded / TotalToDownload;
        // part of the progressmeter that's already "full"
        int dotz = (int)round(fractiondownloaded * totaldotz);

        // create the "meter"
        int ii = 0;
        printf("%3.0f%% [", fractiondownloaded * 100);
        // part  that's full already
        for (; ii < dotz; ii++)
        {
            printf("=");
        }
        // remaining part (spaces)
        for (; ii < totaldotz; ii++)
        {
            printf(" ");
        }
        // and back to line begin - do not forget the fflush to avoid output buffering problems!
        printf("]\r");
        fflush(stdout);
        // if you don't return 0, the transfer will be aborted - see the documentation
        return 0;
    }

    bool downloader::download(std::string const &url, std::string const &outfile)
    {
        CURL *curl;
        CURLcode res;
        FILE *fptr;

        PROCESS("download " << url);

        curl = curl_easy_init();
        if (!curl)
        {
            _error = "Failed to initialize curl";
            return false;
        }

        fptr = fopen((outfile + ".part").c_str(), "wb");
        if (!fptr)
        {
            _error = "Failed to open " + outfile + " for write";
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fptr);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, (getenv("CURL_DEBUG") == nullptr ? 0L : 1L));
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1000);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 10);

        res = curl_easy_perform(curl);

        std::cout << std::endl;

        curl_easy_cleanup(curl);
        fclose(fptr);

        if (res == CURLE_OK)
        {
            std::error_code err;

            std::filesystem::rename(outfile + ".part", outfile, err);
            if (err)
            {
                _error = err.message();
                return false;
            }
        }

        return res == CURLE_OK;
    }

    bool downloader::valid(std::string const &url)
    {
        CURL *curl;
        CURLcode resp;

        curl = curl_easy_init();
        if (!curl)
        {
            _error = "failed to initialize curl";
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, (getenv("CURL_DEBUG") == nullptr ? 0L : 1L));
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1000);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 10);

        resp = curl_easy_perform(curl);

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        curl_easy_cleanup(curl);

        return (resp == CURLE_OK) && http_code == 200;
    }

    bool downloader::get(std::string const &file, std::string const &outdir)
    {
        if (_urls.size() == 0)
        {
            _error = "No url specified";
            return false;
        }

        for (auto const &url : _urls)
        {
            PROCESS("checking " << url << " " << file);

            std::string fileurl = url + "/" + file;

            if (!getenv("NO_CURL_CHECK"))
                if (!valid(fileurl))
                    continue;

            return download(fileurl, outdir);
        }

        _error = file + " is missing on server";

        return false;
    }
}