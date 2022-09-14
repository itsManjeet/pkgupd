#include "downloader.hh"

#include <curl/curl.h>
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
    printf("\033[32;1m▰\033[0m");
  }
  // remaining part (spaces)
  for (; ii < totaldotz; ii++) {
    printf("\033[1m▱\033[0m");
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
  CURL *curl;
  CURLcode res;
  FILE *fptr;

  DEBUG("download " << url);

  curl = curl_easy_init();
  if (!curl) {
    p_Error = "Failed to initialize curl";
    return false;
  }

  auto parent_path = std::filesystem::path(outfile).parent_path();
  if (!std::filesystem::exists(parent_path)) {
    std::error_code err;
    std::filesystem::create_directories(parent_path, err);
    if (err) {
      p_Error = "failed to create required directories " + err.message();
      return false;
    }
  }

  fptr = fopen((string(outfile) + ".part").c_str(), "wb");
  if (!fptr) {
    p_Error = "Failed to open " + string(outfile) + " for write";
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, fptr);
  curl_easy_setopt(curl, CURLOPT_VERBOSE,
                   (getenv("CURL_DEBUG") == nullptr ? 0L : 1L));
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  if (getenv("PKGUPD_NO_PROGRESS") == nullptr) {
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);
  }
  curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1000);
  curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 10);
  

  res = curl_easy_perform(curl);

  std::cout << std::endl;

  curl_easy_cleanup(curl);
  fclose(fptr);

  if (res == CURLE_OK) {
    std::error_code err;

    std::filesystem::rename(string(outfile) + ".part", outfile, err);
    if (err) {
      p_Error = err.message();
      return false;
    }
  }

  return res == CURLE_OK;
}

bool Downloader::valid(char const *url) {
  CURL *curl;
  CURLcode resp;

  curl = curl_easy_init();
  if (!curl) {
    p_Error = "failed to initialize curl";
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
  curl_easy_setopt(curl, CURLOPT_VERBOSE,
                   (getenv("CURL_DEBUG") == nullptr ? 0L : 1L));
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1000);
  curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 10);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, mConfig->get<bool>("downloader.ssl.verify", true) ? 1L : 0L);

  resp = curl_easy_perform(curl);

  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

  curl_easy_cleanup(curl);
  if ((resp == CURLE_OK) && http_code == 200) return true;

  p_Error = "invalid url " + string(url);
  return false;
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

    DEBUG("GET " << fileurl)
    if (!getenv("NO_CURL_CHECK"))
      if (!valid(fileurl.c_str())) continue;

    return download(fileurl.c_str(), outdir);
  }

  p_Error = string(file) + " is missing on server";

  return false;
}
}  // namespace rlxos::libpkgupd