#include "recipe.hh"
#include "compiler/compiler.hh"
#include "installer/installer.hh"
#include "remover/remover.hh"
#include "database/database.hh"

#include <cli/cli.hh>
#include <io.hh>
#include <iostream>

using std::string;

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

        .arg(arg::create("force")
            .long_id("force")
            .short_id('f')
            .about("force task to do"))
        
        .arg(arg::create("recompile-all")
            .long_id("recompile-all")
            .about("recompile all packages in specified recipe file"))

        .arg(arg::create("no-depends")
            .long_id("no-depends")
            .about("skip dependencies resolvement"))

        .sub(app::create("install-file")
            .about("perform installation from filepath")
            .fn([](context const &cc) -> int
            {
                if (cc.args().size() == 0)
                {
                    io::error("no pkgid specified");
                    return 1;
                }

                auto pkgpath = cc.args()[0];
                if (!std::filesystem::exists(pkgpath))
                {
                    io::error(pkgpath, " not exist");
                    return 1;
                }
                auto database = pkgupd::database(cc.config());
                io::debug(level::trace, "configuration:\n", cc.config());

                try 
                {
                    auto [installer, pkg] = pkgupd::installer::frompath(pkgpath, cc.config());
                    string subpkg = "";
                    if (pkg != nullptr)
                        subpkg = pkg->id();

                    if (!installer.install(subpkg))
                    {
                        io::error(installer.error());
                        return false;
                    }

                    if (pkg)
                        delete pkg;
                }
                catch(rlx::obj::exception  e)
                {
                    io::error(e.what());
                    return false;
                }
                

                return true;
            }))

        .sub(app::create("install")
            .about("install specified package from recipe dir")
            .fn([](context const& cc) -> int
            {

                if (cc.args().size() == 0)
                {
                    io::error("no pkgid specified");
                    return 1;
                }

                auto database = pkgupd::database(cc.config());

                io::debug(level::trace, "configuration:\n", cc.config());

                std::vector<string> pkgs;
                
                for(auto  i : cc.args())
                {
                    if (!cc.checkflag("no-depends"))
                    {
                        for(auto const & j : database.resolve(i))
                            pkgs.push_back(j.id());
                    }
                    pkgs.push_back(i);
                }

                for(auto const& pkg : pkgs)
                {
                    io::process("installing ", pkg);                    

                    auto [pkgid, subpkg] = database.parse_pkgid(pkg);

                    try {
                        auto recipe = database[pkgid];
                        auto installer = pkgupd::installer(recipe, cc.config());

                        if (database.installed(pkg) && !cc.checkflag("force"))
                        {
                            io::info(pkg +" is already installed, skipping");
                            continue;
                        }

                        if (!installer.install(subpkg))
                        {
                            io::error(installer.error());
                            return 1;
                        }
                    } 
                    catch(std::runtime_error const& e)
                    {
                        io::error(e.what());
                        return 1;
                    } 
                    catch(pkgupd::database::exception e)
                    {
                        io::message(color::RED, "Database",e.what());
                        return 1;
                    }
                    

                    io::success("installed ", pkg);
                }

                return 0;
            }))
        
        .sub(app::create("depends")
            .about("Resolve required dependencies")
            .fn([](context const& cc )-> int
            {
                if (cc.args().size() == 0)
                {
                    io::error("no pkgid specified");
                    return 1;
                }

                io::debug(level::trace, "configuration:\n", cc.config());
                auto database = pkgupd::database(cc.config());

                try
                {
                    for(auto const& pkg : cc.args())
                        for(auto const& i : database.resolve(pkg))
                            io::println(i.id());
                }
                catch(pkgupd::database::exception e)
                {
                    io::error(e.what());
                    return 1;
                }
                

                return 0;
            }))

        .sub(app::create("compile")
            .about("Compile package from specifed recipe file")
            .fn([](context const& cc) -> int
            {

                if (cc.args().size() == 0)
                {
                    io::error("no pkgid specified");
                    return 1;
                }

                io::debug(level::trace, "configuration:\n", cc.config());
                auto database = pkgupd::database(cc.config());

                std::vector<string> pkgs;
                
                for(auto const& i : cc.args())
                {
                    if (!cc.checkflag("no-depends"))
                    {
                        if (!std::filesystem::exists(i))
                            for(auto const & j : database.resolve(i, true))
                                pkgs.push_back(j.id());
                    }
                       

                    pkgs.push_back(i);
                }

                for(auto const& pkg : pkgs)
                {
                    io::process("compiling ", pkg);                    

                    auto [pkgid, subpkg] = database.parse_pkgid(pkg);
                    
                    try {
                        auto recipe = (std::filesystem::exists(pkg) ? pkgupd::recipe(pkg) : database[pkgid]);
                        auto compiler = pkgupd::compiler(recipe, cc.config());

                        if (database.installed(pkg) && !cc.checkflag("force"))
                        {
                            io::info(pkg, " is already installed/compiled, skipping");
                            continue;
                        }
                        if (subpkg.length() == 0)
                            for(auto const& i : recipe.packages())
                            {
                                if (database.installed(recipe.id()+":"+i.id()) && !cc.checkflag("force"))
                                {
                                    io::info(recipe.id()+":"+i.id(), " is already installed/compiled, skipping");
                                    continue;
                                }
                                if (!compiler.compile(i.id()))
                                {
                                    io::error(compiler.error());
                                    return 1;
                                }
                            }
                        else
                            if (!compiler.compile(subpkg))
                            {
                                io::error(compiler.error());
                                return 1;
                            }

                        
                    }
                    catch(std::runtime_error const& e)
                    {
                        io::error(e.what());
                        return 1;
                    }
                    catch(pkgupd::database::exception e)
                    {
                        io::message(color::RED, "Database", e.what());
                        return 1;
                    }
                    

                    io::success("compiled ", pkg);
                }

                return 0;
            }))

        .sub(app::create("remove")
            .about("Remove specified application from system")
            .fn([](context const& cc) -> int
            {

                if (cc.args().size() == 0)
                {
                    io::error("no pkgid file specified");
                    return 1;
                }

                io::debug(level::trace, "configuration:\n", cc.config());
                auto database = pkgupd::database(cc.config());

                for(auto const& pkg : cc.args())
                {
                    io::process("removing ", pkg);                    

                    auto [pkgid, subpkg] = database.parse_pkgid(pkg);
                    
                    try {
                        if (!database.installed(pkg))
                        {
                            io::info(pkg, " is not already installed, skipping");
                            continue;
                        }

                        auto recipe = database.installed_recipe(pkg);
                        auto remover = pkgupd::remover(recipe, cc.config());

                        if (!remover.remove(pkg))
                        {
                            io::error(remover.error());
                            return 1;
                        }
                    }
                    catch(std::runtime_error const& e)
                    {
                        io::error(e.what());
                        return 1;
                    }
                    catch(pkgupd::database::exception e)
                    {
                        io::message(color::RED, "Database", e.what());
                        return 1;
                    }
                    

                    io::success("removed ", pkg);
                }

                return 0;
                
            }))

        .sub(app::create("info")
            .about("Print Information about pkgid")
            .fn([](context const& cc) -> int
            {
                if (cc.args().size() == 0)
                {
                    io::error("no pkgid file specified");
                    return 1;
                }

                io::debug(level::trace, "configuration:\n", cc.config());
                auto database = pkgupd::database(cc.config());
                try 
                {
                    io::println(database[cc.args()[0]]);
                }
                catch(pkgupd::database::exception e)
                {
                    io::error(e.what());
                    return 1;
                }
                
                return 0;
            }))


        .sub(app::create("sync")
            .about("Synchronize local database with repositories")
            .fn([](context const& cc) -> int
            {
                auto tempfile = rlx::utils::sys::tempfile("/tmp", "rlx-recipes");
                auto database = pkgupd::database(cc.config());

                if (!database.get_from_server("", tempfile))
                {
                    io::error(database.error());
                    return 1;
                }

                YAML::Node node;
                try
                {
                    node = YAML::LoadFile(tempfile);
                    // TODO add verifier
                
                    for(auto const & i : node["recipes"])
                        io::writefile(database.dir_recipe()+"/"+i["id"].as<string>(), i);

                    std::filesystem::remove(tempfile);
                } 
                catch (YAML::BadSubscript e)
                {
                    io::error(e.what());
                    return 1;
                }
                

                

                auto outdated = database.list_out_dated();
                if (outdated.size() != 0)
                {
                    io::info(outdated.size(), " package(s) are outdated");
                    auto ans = io::input(std::cin, "Do you want to continue the update");
                    if (ans == "yes" ||
                        ans == "y" ||
                        ans == "Y" ||
                        ans == "Yes" ||
                        ans == "YES")
                    {
                        for(auto const& i : outdated)
                        {
                            auto updater = pkgupd::installer(i, cc.config());
                            if (!updater.install())
                                io::error(updater.error());
                        }
                        
                    }
                    else
                    {
                        return 0;    
                    }
                }
                return 0;
            }))
        
        .args(ac, av)
        .exec();
}