#ifndef PKGUPD_SOURCE_REPOSITORY_HH
#define PKGUPD_SOURCE_REPOSITORY_HH

#include "configuration.hxx"
#include "recipe.hxx"

namespace libpkgupd {
class SourceRepository : public Object {
 private:
  Configuration* mConfig;
  std::string mRecipeDir;

 public:
  SourceRepository(Configuration* config);
  std::shared_ptr<Recipe> get(char const* id);
};
}  // namespace libpkgupd

#endif