#include "../../recipe.hh"
#include "../../compiler/plugin.hh"
#include <tuple>
#include <string>

#include <io.hh>

using std::string;
using namespace rlx;
using color = rlx::io::color;
using level = rlx::io::debug_level;

class port : public plugin::compiler
{
private:
    string _dir_ports = "/var/cache/recipes";
    std::vector<string> _repo = {"core"};

public:
    port(YAML::Node const &config)
        : plugin::compiler(config)
    {
        if (config["ports"] && config["ports"]["dir"])
            _dir_ports = config["ports"]["dir"].as<string>();

        if (config["ports"] && config["ports"]["repo"])
            for (auto const &i : config["ports"]["repo"])
                _repo.push_back(i.as<string>());
    }

    bool compile(pkgupd::recipe *recipe, pkgupd::package *pkg, string src_dir, string pkg_dir)
    {
        io::debug(level::trace, "port::pkgupd_build() start");
        auto env = recipe->environ(pkg);

        auto checkenv = [&](string i) -> void
        {
            if (_config["compiler"] && _config["compiler"][i])
                env.push_back(io::format(i, "=", _config["compiler"][i].as<string>()));
        };

        string compiler = (_config["compiler"] && _config["compiler"]["recipe"] ? _config["compiler"]["recipe"].as<string>() : "port-compiler");

        string _path;
        for (auto const &i : _repo)
        {
            string p = _dir_ports + "/" + i + "/" + recipe->id();
            io::debug(level::trace, "checking ", p);
            if (std::filesystem::exists(p + "/recipe"))
            {
                _path = p;
                break;
            }
        }

        if (_path.length() == 0)
        {
            _error = "failed to get port path";
            return false;
        }

        for (auto const &i : std::filesystem::directory_iterator(_path))
        {

            string _out_path = src_dir + "/" + i.path().filename().string();
            // check and skip if port already exist
            if (i.path().filename() == "recipe")
            {
                if (std::filesystem::exists(src_dir + "/" + pkg->id()))
                {
                    continue;
                }
                else
                {
                    _out_path = src_dir + "/" + pkg->id();
                }
            }

            std::error_code ec;
            std::filesystem::copy(i.path(), _out_path, ec);
            if (ec)
            {
                _error = ec.message();
                return false;
            }
        }

        io::debug(level::trace, "Compiler: ", color::MAGENTA, compiler, color::RESET);
        if (rlx::utils::exec::command(
                compiler + " " + pkg->id(), src_dir,
                env))
        {
            _error = "failed to execute recipe compiler";
            return false;
        }
        return true;
    }
};

extern "C" plugin::compiler *pkgupd_init(YAML::Node const &config)
{
    return new port(config);
}