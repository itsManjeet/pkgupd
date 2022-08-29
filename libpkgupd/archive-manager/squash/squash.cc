#include "squash.hh"

#include <sys/stat.h>
#include <unistd.h>

#include <fstream>

#include "../../utils/utils.hh"

using namespace rlxos::libpkgupd;

#include <string>
using namespace std;

bool Squash::get(char const* imagefile, char const* input_path,
                 std::string& output) {
  string cmd = "/bin/unsquashfs";
  cmd += " -o ";
  cmd += mOffset;
  cmd += " ";
  cmd += " -cat ";
  cmd += " ";
  cmd += imagefile;
  cmd += " ";
  cmd += input_path;

  auto [status, out] = Executor::output(cmd);
  if (status != 0) {
    p_Error = "failed to get data from " + string(imagefile);
    return false;
  }
  output = out;
  return true;
}

bool Squash::extract_file(char const* imagefile, char const* input_path,
                          char const* output_path) {
  string cmd = "/bin/unsquashfs";
  cmd += " -o ";
  cmd += mOffset;
  cmd += " ";
  cmd += " -cat";
  cmd += " ";
  cmd += imagefile;
  cmd += " ";
  cmd += input_path;
  cmd += " >";
  cmd += output_path;

  auto [status, out] = Executor::output(cmd);
  if (status != 0) {
    p_Error = "failed to get data from " + string(imagefile) + ", " + out;
    return false;
  }
  return true;
}

shared_ptr<PackageInfo> Squash::info(char const* input_path) {
  std::string content;
  if (!get(input_path, "info", content)) {
    return nullptr;
  }

  shared_ptr<PackageInfo> pkginfo = nullptr;
  try {
    auto node = YAML::Load(content);
    pkginfo = make_shared<PackageInfo>(node, input_path);
  } catch (exception const& exc) {
    p_Error = exc.what();
  }
  return pkginfo;
}

bool Squash::list(char const* input_path, vector<string>& files) {
  string cmd = "/bin/unsquashfs";
  cmd += " -o ";
  cmd += mOffset;
  cmd += " -l ";
  cmd += input_path;

  auto [status, output] = Executor::output(cmd);
  if (status != 0) {
    p_Error = output;
    return false;
  }

  stringstream ss(output);
  string file;

  while (getline(ss, file, '\n')) {
    files.push_back(file);
  };
  return true;
}

bool Squash::extract(char const* input_path, char const* output_path,
                     std::vector<std::string>&) {
  std::string cmd = "/bin/unsquashfs";
  cmd += " -o ";
  cmd += mOffset;
  cmd += " -f -d ";
  cmd += output_path;
  cmd += " ";
  cmd += input_path;
  cmd += " -exclude-file info";

  if (Executor::execute(cmd) != 0) {
    p_Error = "failed to execute extraction command";
    return false;
  }

  return true;
}

bool Squash::compress(char const* input_path, char const* src_dir) {
  std::string cmd = "/bin/mksquashfs ";
  cmd += src_dir;
  cmd += "/* ";
  cmd += input_path;
  cmd += " -comp zstd ";
  cmd += " -b 256K ";
  cmd += " -noappend ";
  cmd += " -Xcompression-level 22";

  if (Executor::execute(cmd) != 0) {
    p_Error = "failed to execute command for compression '" + cmd + "'";
    return false;
  }

  return true;
}