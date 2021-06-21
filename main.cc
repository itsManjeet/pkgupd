#include "recipe.hh"
#include "compiler/compiler.hh"
#include "installer/installer.hh"
#include "database/database.hh"

#include <cli/cli.hh>
#include <io.hh>

std::tuple<std::string, std::string> parse_pkgid(std::string pkgid)
{
    size_t idx = pkgid.find_first_of(':');
    if (idx== std::string::npos)
        return {pkgid, ""};

    return {pkgid.substr(0,idx), pkgid.substr(idx+1, pkgid.length() - (idx+1))};
}

int main(int ac, char **av)
{
    namespace io = rlx::io;
    using color = rlx::io::color;
    using level = rlx::io::debug_level;

    using namespace rlx::cli;

    return app::create("pkgupd")
        .version("0.1.0")
        .about("an extensible package manager for rlxos")

        .usage("[Sub] <pkgid> <Args>")

        .config({"/etc/pkgupd.yml", "/usr/etc/pkgupd.yml", "pkgupd.yml"})

        .arg(arg::create("help")
                 .short_id('h')
                 .long_id("help")
                 .fn([](context const &cc) -> int
                    {
                        io::println(*cc.getapp());
                        return 0;
                    }))

        .arg(arg::create("debug")
            .short_id('d')
            .long_id("debug")
            .required(true))

        .arg(arg::create("recompile")
            .long_id("recompile")
            .about("recompile specified package if already compiled")
            .required(true))
        
        .arg(arg::create("recompile-all")
            .long_id("recompile-all")
            .about("recompile all packages in specified recipe file"))

        .sub(app::create("install")
            .about("install specified package from recipe dir")
            .fn([](context const& cc) -> int
            {

                if (cc.args().size() == 0)
                {
                    io::error("no recipe file specified");
                    return 1;
                }

                io::debug(level::trace, "configuration:\n", cc.config());

                for(auto const& pkg : cc.args())
                {
                    io::process("installing ", pkg);                    

                    auto [pkgid, subpkg] = parse_pkgid(pkg);

                    auto recipe = pkgupd::recipe(pkgid);
                    auto installer = pkgupd::installer(recipe, cc.config());

                    try {
                        if (!installer.install(subpkg))
                        {
                            io::error(installer.error());
                            return 1;
                        }
                    } catch(std::runtime_error const& e)
                    {
                        io::error(e.what());
                        return 1;
                    }
                    

                    io::success("installed ", pkg);
                }

                return 0;
            }))

        .sub(app::create("compile")
            .about("Compile package from specifed recipe file")
            .fn([](context const& cc) -> int
            {

                if (cc.args().size() == 0)
                {
                    io::error("no recipe file specified");
                    return 1;
                }

                io::debug(level::trace, "configuration:\n", cc.config());

                for(auto const& pkg : cc.args())
                {
                    io::process("compiling ", pkg);                    

                    auto [pkgid, subpkg] = parse_pkgid(pkg);


                    auto recipe = pkgupd::recipe(pkgid);
                    auto compiler = pkgupd::compiler(recipe, cc.config());

                    try {
                        if (!compiler.compile(subpkg))
                        {
                            io::error(compiler.error());
                            return 1;
                        }
                    } catch(std::runtime_error const& e)
                    {
                        io::error(e.what());
                        return 1;
                    }
                    

                    io::success("compiled ", pkg);
                }

                return 0;
            }))
        
        .args(ac, av)
        .exec();
}