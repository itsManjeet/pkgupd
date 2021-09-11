#include "Downloader.hh"
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

    bool Downloader::performCurl(std::string const &url, std::string const &outfile)
    {
        CURL *curl;
        CURLcode res;
        FILE *fptr;

        std::cout << "=> downloading " << url << std::endl;

        curl = curl_easy_init();
        if (!curl)
        {
            error = "Failed to initialize curl";
            return false;
        }

        fptr = fopen((outfile + ".part").c_str(), "wb");
        if (!fptr)
        {
            error = "Failed to open " + outfile + " for write";
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fptr);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, (getenv("DEBUG") == nullptr ? 0L : 1L));
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);

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
                error = err.message();
                return false;
            }
        }

        return res == CURLE_OK;
    }

    bool Downloader::isExist(std::string const &url)
    {
        CURL *curl;
        CURLcode resp;

        curl = curl_easy_init();
        if (!curl)
        {
            error = "failed to initialize curl";
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
        resp = curl_easy_perform(curl);

        curl_easy_cleanup(curl);

        return (resp == CURLE_OK);
    }

    bool Downloader::Download(std::string const &file, std::string const &outdir)
    {
        if (urls.size() == 0)
        {
            error = "No url specified";
            return false;
        }

        bool isDownloaded = false;
        for (auto const &url : urls)
        {
            std::cout << "=> checking " << url << " " << file << std::endl;
            std::string fileurl = url + "/" + file;
            if (!isExist(fileurl))
                continue;

            return performCurl(fileurl, outdir);
        }

        error = file + " is missing on server, please report to admin at " + BUG_URL;

        return isDownloaded;
    }
}