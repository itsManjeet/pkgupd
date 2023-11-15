#include "source-repository.hxx"

using namespace rlxos::libpkgupd;

#include <string>
#include <vector>

inline bool ends_with(std::string const &value, std::string const &ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

SourceRepository::SourceRepository(Configuration *config) : mConfig(config) {
    mRecipeDir = mConfig->get<std::string>(DIR_RECIPES, DEFAULT_RECIPE_DIR);
}

std::shared_ptr<Recipe> SourceRepository::get(char const *id) {
    std::vector<std::string> repos;

    std::filesystem::path required_path = (std::string(id) + ".yml");

    auto recipefile = std::filesystem::path(mRecipeDir) / (std::string(id) + ".yml");
    if (std::filesystem::exists(recipefile)) {
        YAML::Node node = YAML::LoadFile(recipefile.string());
        return std::make_shared<Recipe>(node, recipefile.string());
    }

    return nullptr;
}