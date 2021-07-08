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
public:
    port(YAML::Node const &config)
        : plugin::compiler(config)
    {
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

extern "C" plugin::compiler *pkgupd_build(YAML::Node const &config)
{
    return new port(config);
}