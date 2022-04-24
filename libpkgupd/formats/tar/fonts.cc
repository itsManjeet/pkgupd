#include "fonts.hh"

using namespace rlxos::libpkgupd;

bool Fonts::extract(std::string const &outdir) {
  if (!std::filesystem::exists(m_PackageFile)) {
    p_Error = "no " + m_PackageFile + " exists";
    return false;
  }

  std::string fonts_dir = outdir + "/opt/share/fonts/" + m_Package.id();
  if (!std::filesystem::exists(fonts_dir)) {
    std::error_code error;
    std::filesystem::create_directories(fonts_dir, error);
    if (error) {
      p_Error =
          "failed to create font_dir " + fonts_dir + ", " + error.message();
      return false;
    }
  }

  std::string cmd = "/bin/tar";
  cmd += " --zstd --exclude './info' -xPhpf " + m_PackageFile + " -C " + fonts_dir;
  if (Executor().execute(cmd) != 0) {
    p_Error = "failed to execute extraction command";
    return false;
  }

  return true;
}