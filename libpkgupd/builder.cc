#include "builder.hh"

#include <string.h>
#include <sys/stat.h>

#include <fstream>
#include <vector>

#include "archive-manager/archive-manager.hh"
#include "bundler.hh"
#include "colors.hh"
#include "compilers/autoconf.hh"
#include "compilers/cargo.hh"
#include "compilers/cmake.hh"
#include "compilers/go.hh"
#include "compilers/makefile.hh"
#include "compilers/meson.hh"
#include "compilers/pysetup.hh"
#include "compilers/script.hh"
#include "downloader.hh"
#include "exec.hh"
#include "recipe.hh"
#include "stripper.hh"

namespace fs = std::filesystem;
using fs::path;
using std::string;
namespace rlxos::libpkgupd {

static std::vector<std::string> HOT_FILES_LIST = {
#define X(ID, NAME, FILE) FILE,
    BUILD_TYPE_LIST
#undef X
};

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

    auto sourcefile_Path = path(mSourceDir) / sourcefile;

    auto downloader = Downloader(mConfig);
    if (!fs::exists(sourcefile_Path)) {
      if (!downloader.download(url.c_str(), sourcefile_Path.c_str())) {
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

bool Builder::build(Recipe *recipe) {
  auto srcdir = path(mBuildDir) / "src";
  auto pkgdir = path(mBuildDir) / "pkg" / recipe->id();

  for (auto dir :
       std::vector<std::filesystem::path>{srcdir, (pkgdir).parent_path()}) {
    std::error_code err;
    DEBUG("creating required dir: " << dir)
    fs::create_directories(dir, err);
    if (err) {
      p_Error = "failed to create required build directories " + err.message();
      return false;
    }
  }

  if (!mConfig->get("build.skip-prepare", false)) {
    if (!prepare(recipe->sources(), srcdir)) {
      return false;
    }
  }

  auto environ = recipe->environ();
  environ.push_back("PKGUPD_SRCDIR=" + mSourceDir);
  environ.push_back("PKGUPD_PKGDIR=" + mPackageDir);
  environ.push_back("pkgupd_srcdir=" + srcdir.string());
  environ.push_back("pkgupd_pkgdir=" + pkgdir.string());
  environ.push_back("DESTDIR=" + pkgdir.string());
  mConfig->get("environ", environ);

  std::filesystem::path wrkdir;
  std::string build_work_type =
      mConfig->get<std::string>("build.work.type", "default");
  if (build_work_type == "default") {
    wrkdir = srcdir / recipe->buildDir();
    if (recipe->buildDir().length() == 0 && recipe->sources().size()) {
      auto [status, output] = Executor().output(
          "tar -taf " +
              std::filesystem::path(recipe->sources()[0]).filename().string() +
              " | head -n1",
          mSourceDir);
      if (status != 0 || output.length() == 0) {
      } else {
        if (output[output.length() - 1] == '\n') {
          output = output.substr(0, output.length() - 1);
        }
        wrkdir = srcdir / output;
      }
    }
  } else if (build_work_type == "local") {
    wrkdir = std::filesystem::path(
                 mConfig->get<std::string>("build.recipe", "recipe.yml"))
                 .parent_path();
  }
  if (recipe->prescript().size()) {
    PROCESS("executing prescript")
    if (int status =
            Executor().execute(recipe->prescript(), wrkdir.string(), environ);
        status != 0) {
      p_Error = "prescript failed with exit code: " + std::to_string(status);
      return false;
    }
  }

  PROCESS("compiling source code")

  if (!compile(recipe, wrkdir, pkgdir, environ)) {
    return false;
  }

  if (recipe->postscript().size()) {
    PROCESS("executing postscript")
    if (int status =
            Executor().execute(recipe->postscript(), wrkdir.string(), environ);
        status != 0) {
      p_Error = "postscript failed with exit code: " + std::to_string(status);
      return false;
    }
  }
  for (auto const &i : std::filesystem::recursive_directory_iterator(pkgdir)) {
    if (i.is_regular_file() && i.path().filename().extension() == ".la") {
      DEBUG("removing " + i.path().string());
      std::filesystem::remove(i);
    }
  }

  if (recipe->dostrip()) {
    Stripper stripper(recipe->skipStrip());
    PROCESS("stripping package");
    if (!stripper.strip(pkgdir)) {
      ERROR(stripper.error());
    }
  }

  std::ofstream file(pkgdir / "info");
  (*recipe)[recipe->id()]->dump(file);
  file.close();

  for (auto const &file : recipe->node()["files"]) {
    std::ofstream f(pkgdir / file["path"].as<string>());
    f << file["content"].as<string>();
    f.close();
    if (file["perm"]) {
      chmod((pkgdir / file["path"].as<string>()).c_str(),
            file["perm"].as<int>());
    }
  }

  if (recipe->node()["bundle"] && recipe->node()["bundle"].as<bool>()) {
    Bundler bunder = Bundler(pkgdir, "/");
    std::vector<std::string> exclude;
    for (auto const &lib : recipe->node()["exclude-libraries"]) {
      exclude.push_back(lib.as<std::string>());
    }
    mConfig->get("bindler.exclude", exclude);
    // TODO: add list of libraries to include and exclude
    if (!bunder.resolveLibraries(exclude)) {
      p_Error = "Failed to bundle libraries, " + bunder.error();
      return false;
    }
  }

  std::vector<std::pair<std::shared_ptr<PackageInfo>, std::string>> packagesdir;
  packagesdir.push_back({(*recipe)[recipe->id()], pkgdir});

  for (auto const &split : recipe->splits()) {
    std::error_code err;
    auto splitdir_Path = path(mBuildDir) / "pkg" / split.into;
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

      fs::copy(srcfile_Path, destfile_Path, fs::copy_options::copy_symlinks,
               err);
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

    (*recipe)[id]->dump(file);
    file.close();

    packagesdir.push_back({(*recipe)[id], splitdir_Path});
  }

  if (!pack(packagesdir)) {
    return false;
  }

  return true;
}

std::shared_ptr<Compiler> Compiler::create(BuildType buildType) {
  switch (buildType) {
#define X(id, name, file) \
  case BuildType::id:     \
    return std::make_shared<id>();
    BUILD_TYPE_LIST
#undef X
  }
  return nullptr;
}

bool Builder::compile(Recipe *recipe, std::string dir, std::string destdir,
                      std::vector<std::string> &environ) {
  std::shared_ptr<Compiler> compiler;
  BuildType type;
  if (recipe->buildType() != BuildType::N_BUILD_TYPE) {
    type = recipe->buildType();
  } else if (recipe->script().size() != 0) {
    type = BuildType::Script;
  } else {
    type = DETECT_BUILD_TYPE(dir);
  }
  compiler = Compiler::create(type);
  if (compiler == nullptr) {
    p_Error = "unsupported compiler for type used '" +
              std::string(BUILD_TYPE_STR[int(type)]) + "'";
    return false;
  }

  if (!compiler->compile(recipe, mConfig, dir.c_str(), destdir.c_str(),
                         environ)) {
    p_Error = compiler->error();
    return false;
  }

  return true;
}

bool Builder::pack(
    std::vector<std::pair<std::shared_ptr<PackageInfo>, std::string>> const
        &dirs) {
  for (auto const &i : dirs) {
    auto packagefile_Path = std::filesystem::proximate(mPackageDir) /
                            i.first->repository() / (PACKAGE_FILE(i.first));

    auto archive_manager = ArchiveManager::create(i.first->type());
    if (archive_manager == nullptr) {
      p_Error = "no suitable archive manager found for type '" +
                std::string(PACKAGE_TYPE_STR[int(i.first->type())]) + "'";
      return false;
    }

    if (!std::filesystem::exists(packagefile_Path.parent_path())) {
      std::error_code error;
      std::filesystem::create_directories(packagefile_Path.parent_path());
      if (error) {
        p_Error = "failed to create require directory '" +
                  packagefile_Path.parent_path().string() + "', " +
                  error.message();
        return false;
      }
    }
    if (!archive_manager->compress(packagefile_Path.c_str(),
                                   i.second.c_str())) {
      p_Error = "compression failed " + archive_manager->error();
      return false;
    }
  }

  return true;
}

}  // namespace rlxos::libpkgupd