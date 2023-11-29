#include "source-repository.hxx"

using namespace rlxos::libpkgupd;

#include <string>
#include <vector>

inline bool ends_with(std::string const& value, std::string const& ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

SourceRepository::SourceRepository(Configuration* config) : mConfig(config) {
    mRecipeDir = mConfig->get<std::string>(DIR_RECIPES, DEFAULT_RECIPE_DIR);
}

std::optional<Recipe> SourceRepository::get(const std::string& id) {
    if (auto const recipe_path = std::filesystem::path(mRecipeDir) / id; std::filesystem::exists(recipe_path)) {
        return Recipe(YAML::LoadFile(recipe_path));
    }

    return std::nullopt;
}
