#include "source-repository.hh"
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

std::shared_ptr<Recipe> SourceRepository::get(char const* id) {
    std::vector<std::string> repos;
    mConfig->get(REPOS, repos);

    std::filesystem::path required_path = (std::string(id) + ".yml");

    for (auto const& i : repos) {
        std::filesystem::path repo_path = std::filesystem::path(mRecipeDir) / i;
        if (!std::filesystem::exists(repo_path)) continue;

        auto recipefile = repo_path / id / (std::string(id) + ".yml");
        if (std::filesystem::exists(recipefile)) {
            YAML::Node node = YAML::LoadFile(recipefile.string());
            return std::make_shared<Recipe>(node, recipefile.string(), i);
        }
    }
    return nullptr;
}