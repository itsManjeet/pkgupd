#ifndef __REMOVER__
#define __REMOVER__

#include <rlx.hh>
#include <algo/algo.hh>
#include "../recipe.hh"
#include "../database/database.hh"
#include "../config.hh"

namespace pkgupd
{
    using namespace rlx;
    using color = rlx::io::color;
    using level = rlx::io::debug_level;

    class remover : public obj
    {
    private:
        recipe _recipe;
        YAML::Node _config;
        database _database;

    public:
        remover(recipe const &r,
                YAML::Node const &c)
            : _recipe(r), _config(c), _database(c)
        {
        }

        ~remover()
        {
        }

        bool remove(string pkgid)
        {
            assert(_database.installed(pkgid));
            auto pkgfiles = _database.installedfiles(pkgid);

            std::reverse(pkgfiles.begin(), pkgfiles.end());

            for (auto const &i : pkgfiles)
            {
                string fpath = _database.dir_root() + "/" + i;
                if (std::filesystem::exists(fpath))
                {
                    if (std::filesystem::is_directory(fpath) && !std::filesystem::is_empty(fpath))
                        continue;

                    if (!std::filesystem::remove(fpath))
                        io::warn("failed to remove, fpath");
                }
            }

            if (!std::filesystem::remove(_database.dir_data() + "/" + pkgid))
            {
                _error = "failed to remove data file for " + pkgid;
                return false;
            }

            if (!_database.exec_triggers(pkgfiles))
            {
                _error = _database.error();
                return false;
            }

            return true;
        }
    };
}

#endif