#ifndef _PKGUPD_REPOSITORY_DATABASE_HH_
#define _PKGUPD_REPOSITORY_DATABASE_HH_

#include "database.hh"
#include "recipe.hh"

namespace rlxos::libpkgupd {
class Repository : public Database {
 private:
  std::vector<std::string> mRepos;

 public:
  Repository(std::string const &dataDir, std::vector<std::string> const& repos) : Database(dataDir), mRepos(repos) {
    DEBUG("Repository Database: " << dataDir);
  }

  std::vector<std::string> repos() const {
    return mRepos;
  }

  std::vector<Recipe> recipes(std::string const& repo);

  std::vector<Package> all();

  std::optional<Recipe> recipe(std::string const &pkgid);
  std::optional<Package> operator[](std::string const &pkgid);
};
}  // namespace rlxos::libpkgupd

#endif