#include "theme.hh"

using namespace rlxos::libpkgupd;

bool Themes::extract(std::string const &outdir) {
  if (!std::filesystem::exists(m_PackageFile)) {
    p_Error = "no " + m_PackageFile + " exists";
    return false;
  }

  std::string theme_dir = outdir + "/opt/share/themes/" + m_Package.id();
  if (!std::filesystem::exists(theme_dir)) {
    std::error_code error;
    std::filesystem::create_directories(theme_dir, error);
    if (error) {
      p_Error =
          "failed to create theme_dir " + theme_dir + ", " + error.message();
      return false;
    }
  }

  std::string cmd = "/bin/tar";
  cmd += " --zstd --exclude './info' -xPhpf " + m_PackageFile + " -C " + theme_dir;
  if (Executor().execute(cmd) != 0) {
    p_Error = "failed to execute extraction command";
    return false;
  }

  return true;
}