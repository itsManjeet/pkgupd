#include "builder.hh"

#include <fstream>
#include <vector>

#include "colors.hh"
#include "compilers/autoconf.hh"
#include "compilers/cmake.hh"
#include "compilers/meson.hh"
#include "compilers/script.hh"
#include "downloader.hh"
#include "exec.hh"
#include "packager.hh"
#include "recipe.hh"
#include "stripper.hh"

namespace fs = std::filesystem;
using fs::path;
using std::string;
namespace rlxos::libpkgupd {

bool Builder::prepare(std::vector<std::string> const &sources,
                      std::string const &dir) {
  for (auto const &source : sources) {
    auto sourcefile = path(source).filename().string();
    auto url = source;

    auto index = source.find("::");
    if (index != string::npos) {
      sourcefile = source.substr(0, index);
      url = source.substr(index + 2, source.length() - (index + 2));
    }

    auto sourcefile_Path = path(m_SourceDir) / sourcefile;

    auto downloader = Downloader({}, "");
    if (!fs::exists(sourcefile_Path)) {
      if (!downloader.download(url, sourcefile_Path)) {
        p_Error =
            "failed to download '" + sourcefile + "' " + downloader.error();
        return false;
      }
    }

    auto endswith = [](std::string const &fullstr, std::string const &ending) {
      if (fullstr.length() >= ending.length())
        return (0 == fullstr.compare(fullstr.length() - ending.length(),
                                     ending.length(), ending));
      else
        return false;
    };

    bool extracted = false;
    for (auto const &i : {".tar", ".gz", ".tgz", ".xz", ".txz", ".bzip2", ".bz",
                          ".bz2", ".lzma"}) {
      if (endswith(sourcefile, i)) {
        PROCESS("extracting " << sourcefile)
        if (int status = Executor().execute(
                "tar -xPf " + sourcefile_Path.string() + " -C " + dir);
            status != 0) {
          p_Error = "failed to extract " + sourcefile_Path.string() +
                    " with tar, exit status: " + std::to_string(status);
          return false;
        }
        extracted = true;
        break;
      }
    }

    if (path(sourcefile_Path).extension() == ".zip") {
      if (int status = Executor().execute("unzip " + sourcefile_Path.string() +
                                          " -d " + dir);
          status != 0) {
        p_Error = "failed to extract " + sourcefile_Path.string() +
                  " with unzip, exit status: " + std::to_string(status);
        return false;
      }
      extracted = true;
    }

    if (!extracted) {
      std::error_code err;

      fs::copy(sourcefile_Path, path(dir) / sourcefile_Path.filename(), err);
      if (err) {
        p_Error = "failed to copy " + sourcefile_Path.string() + ", error " +
                  err.message();
        return false;
      }
    }
  }

  return true;
}

bool Builder::build(Recipe const &recipe) {
  auto srcdir = path(m_BuildDir) / "src";
  auto pkgdir = path(m_BuildDir) / "pkg" / recipe.id();

  for (auto dir : std::vector<std::filesystem::path>{srcdir, pkgdir}) {
    std::error_code err;
    DEBUG("creating required dir: " << dir)
    fs::create_directories(dir, err);
    if (err) {
      p_Error = "failed to create required build directories " + err.message();
      return false;
    }
  }

  if (!prepare(recipe.sources(), srcdir)) {
    return false;
  }

  auto environ = recipe.environ();
  environ.push_back("PKGUPD_SRCDIR=" + m_SourceDir);
  environ.push_back("PKGUPD_PKGDIR=" + m_PackageDir);
  environ.push_back("pkgupd_srcdir=" + pkgdir.string());
  environ.push_back("pkgupd_pkgdir=" + pkgdir.string());
  environ.push_back("DESTDIR=" + pkgdir.string());

  auto wrkdir = srcdir / recipe.buildDir();

  if (recipe.prescript().size()) {
    PROCESS("executing prescript")
    if (int status =
            Executor().execute(recipe.prescript(), wrkdir.string(), environ);
        status != 0) {
      p_Error = "prescript failed with exit code: " + std::to_string(status);
      return false;
    }
  }

  PROCESS("compiling source code")

  if (!compile(recipe, wrkdir, pkgdir, environ)) {
    return false;
  }

  if (recipe.postscript().size()) {
    PROCESS("executing postscript")
    if (int status =
            Executor().execute(recipe.postscript(), wrkdir.string(), environ);
        status != 0) {
      p_Error = "postscript failed with exit code: " + std::to_string(status);
      return false;
    }
  }
  for (auto const &i : std::filesystem::recursive_directory_iterator(wrkdir)) {
    if (i.is_regular_file() && i.path().filename().extension() == ".la") {
      DEBUG("removing " + i.path().string());
      std::filesystem::remove(i);
    }
  }

  if (recipe.dostrip()) {
    Stripper stripper(recipe.skipStrip());
    PROCESS("stripping package");
    if (!stripper.strip(wrkdir)) {
      ERROR(stripper.error());
    }
  }

  std::ofstream file(pkgdir / "info");
  recipe[recipe.id()]->dump(file);
  file.close();

  std::vector<std::string> packagesdir;
  packagesdir.push_back(pkgdir);

  for (auto const &split : recipe.splits()) {
    std::error_code err;
    auto splitdir_Path = path(m_BuildDir) / "pkg" / split.into;
    fs::create_directories(splitdir_Path, err);
    if (err) {
      p_Error = "failed to create split dir " + err.message();
      return false;
    }

    for (auto const &file : split.files) {
      auto srcfile_Path = pkgdir / file;
      auto destfile_Path = splitdir_Path / file;
      PROCESS("creating " << destfile_Path);
      fs::create_directories(destfile_Path.parent_path(), err);
      if (err) {
        p_Error = "failed to create required dir " + err.message();
        return false;
      }

      fs::copy_file(srcfile_Path, destfile_Path, err);
      if (err) {
        p_Error = "failed to copy file " + file + " " + err.message();
        return false;
      }

      fs::remove(srcfile_Path, err);
      if (err) {
        p_Error = "failed to clean split file " + file + " " + err.message();
        return false;
      }
    }

    std::ofstream file(splitdir_Path / "info");
    auto id = split.into;
    if (id == "lib") {
      id += recipe.id();
    }

    recipe[id]->dump(file);
    file.close();

    packagesdir.push_back(splitdir_Path);
  }

  if (!pack(packagesdir)) {
    return false;
  }

  return true;
}

std::shared_ptr<Compiler> Compiler::create(BuildType buildType) {
  switch (buildType) {
    case BuildType::CMAKE:
      return std::make_shared<Cmake>();
    case BuildType::MESON:
      return std::make_shared<Meson>();
    case BuildType::AUTOCONF:
      return std::make_shared<AutoConf>();
    case BuildType::SCRIPT:
      return std::make_shared<Script>();
  }
  throw std::runtime_error("unsupported " + buildTypeToString(buildType));
}

bool Builder::compile(Recipe const &recipe, std::string dir,
                      std::string destdir,
                      std::vector<std::string> const &environ) {
  std::shared_ptr<Compiler> compiler;

  if (recipe.buildType() != BuildType::INVALID) {
    compiler = Compiler::create(recipe.buildType());
  } else if (recipe.script().size() != 0) {
    compiler = Compiler::create(BuildType::SCRIPT);
  } else {
    try {
      auto buildType = detectBuildType(dir);
      compiler = Compiler::create(buildType);
    } catch (std::runtime_error const &err) {
      p_Error = err.what();
      return false;
    }
  }

  if (!compiler->compile(recipe, dir, destdir, environ)) {
    p_Error = compiler->error();
    return false;
  }

  return true;
}

bool Builder::pack(std::vector<std::string> const &dirs) {
  for (auto const &i : dirs) {
    auto node = YAML::LoadFile(i + "/info");
    auto package = Package(node, i + "/info");

    auto packagefile_Path = m_PackageDir + "/" + package.file();

    auto packager = Packager::create(package.type(), packagefile_Path);
    if (!packager->compress(i, package)) {
      p_Error = "failed to compress " + packager->error();
      return false;
    }
  }

  return true;
}

}  // namespace rlxos::libpkgupd