#include "bundler.hh"

#include <algorithm>
#include <set>
using namespace rlxos::libpkgupd;

bool Bundler::resolveLibraries(std::vector<std::string> const& except) {
  std::set<std::string> requiredLibraries;

  for (auto const& d :
       std::filesystem::recursive_directory_iterator(m_WorkDir)) {
    if (d.is_directory()) {
      continue;
    }

    std::string mimeType = Bundler::mime(d.path());
    if (mimeType == "application/x-executable" ||
        mimeType == "application/x-sharedlib") {
      auto localLibraries = ldd(d.path());
      requiredLibraries.insert(localLibraries.begin(), localLibraries.end());
    }
  }

  for (auto const& i : requiredLibraries) {
    auto libname = std::filesystem::path(i).filename().string();
    if (std::find(except.begin(), except.end(), libname) == except.end()) {
      DEBUG("adding " << i);
      std::error_code err;
      if (i == m_WorkDir + "/usr/lib/" + libname) {
        continue;
      }
      if (std::filesystem::exists(m_WorkDir + "/usr/lib/" + libname)) {
        continue;
      }
      std::filesystem::copy_file(i, m_WorkDir + "/usr/lib/" + libname, err);
      if (err) {
        p_Error = "failed to install " + i + ", " + err.message();
        return false;
      }
    }
  }
  return true;
}

std::set<std::string> Bundler::ldd(std::string path) {
  std::string libenv;
  libenv = "LD_LIBRARY_PATH=" + m_WorkDir + ":" + m_WorkDir +
           "/usr/lib:" + m_RootDir + ":" + m_RootDir + "/usr/lib";
  auto [status, output] = Executor().output(
      "ldd " + path + " 2>&1 | awk '{print $3}'", ".", {libenv});

  std::stringstream ss(output);
  std::set<std::string> libraryList;
  std::string lib;

  while (std::getline(ss, lib, '\n')) {
    if (lib.length() == 0 || lib == "not") {
      continue;
    }
    DEBUG("found " + lib);
    libraryList.insert(lib);
  }
  return libraryList;
}

std::string Bundler::mime(std::string path) {
  auto [status, output] =
      Executor().output("file --mime-type " + path + " | awk '{print $2}'");
  if (status != 0) {
    throw std::runtime_error("failed to get mime type for " + path + ", " +
                             output);
  }
  return output.substr(0, output.size() - 1);
}