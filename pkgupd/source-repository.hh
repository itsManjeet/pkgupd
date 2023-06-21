#ifndef PKGUPD_SOURCE_REPOSITORY_HH
#define PKGUPD_SOURCE_REPOSITORY_HH

#include "configuration.hh"
#include "recipe.hh"

namespace rlxos::libpkgupd {
class SourceRepository : public Object {
   private:
    Configuration* mConfig;
    std::string mRecipeDir;

   public:
    SourceRepository(Configuration* config);
    std::shared_ptr<Recipe> get(char const* id);
};
}  // namespace rlxos::libpkgupd

#endif