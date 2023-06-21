#include "appimage.hxx"

using namespace rlxos::libpkgupd;

#include <sys/stat.h>

#include "../../exec.hxx"

bool AppImage::get_offset(char const *path, std::string &offset) {
    if (chmod(path, 0755) == -1) {
        offset = "failed to set executable permission " + std::string(path);
        return false;
    }

    auto [status, output] =
            Executor::output(std::string(path) + " --appimage-offset");
    if (status != 0) {
        offset = output;
        return false;
    }

    offset = output;
    return true;
}

bool AppImage::get(char const *app_image, char const *input_path,
                   std::string &output) {
    if (!get_offset(app_image, mOffset)) {
        p_Error = mOffset;
        return false;
    }
    return Squash::get(app_image, input_path, output);
}

std::shared_ptr<PackageInfo> AppImage::info(char const *input_path) {
    if (!get_offset(input_path, mOffset)) {
        p_Error = mOffset;
        return nullptr;
    }
    return Squash::info(input_path);
}

bool AppImage::list(char const *input_path, std::vector<std::string> &files) {
    if (!get_offset(input_path, mOffset)) {
        p_Error = mOffset;
        return false;
    }
    return Squash::list(input_path, files);
}

bool AppImage::extract(char const *input_path, char const *output_path,
                       std::vector<std::string> &files) {
    if (!get_offset(input_path, mOffset)) {
        p_Error = mOffset;
        return false;
    }
    return Squash::extract(input_path, output_path, files);
}

bool AppImage::compress(char const *input_path, char const *src_dir) {
    std::string cmd = "ARCH=x86_64 appimagetool ";
    cmd += src_dir;
    cmd += " ";
    cmd += input_path;
    if (Executor::execute(cmd) != 0) {
        p_Error = "failed to compress appimage";
        return false;
    }
    return true;
}
