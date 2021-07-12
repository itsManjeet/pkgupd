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

        .arg(arg::create("compile")
            .long_id("compile")
            .about("compile specified package if already compiled"))

        .arg(arg::create("force")
            .long_id("force")
            .short_id('f')
            .about("force task to do"))

        .arg(arg::create("no-depends")
            .long_id("no-depends")
            .about("skip dependencies resolvement"))
        
        .arg(arg::create("all")
            .short_id('a')
            .long_id("all")
            .about("Set ALL flag"))

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
                auto installer = pkgupd::installer(cc.config());
                auto compiler = pkgupd::compiler(cc.config());

                io::debug(level::trace, "configuration:\n", cc.config());

                string pkgid = cc.args()[0];

                if (!cc.checkflag("no-depends"))
                {
                    auto dep = database.resolve(pkgid, cc.checkflag("compile"));
                    dep.pop_back();
                    for(auto i : dep)
                    {
                        auto subcc = context(cc);
                        subcc.args({i});
                        subcc.flags({"no-depends"});
                        if (cc.exec("install", subcc) != 0)
                        {
                            return 1;
                        }
                    }
                }

                std::vector<string> packages;
                pkgupd::recipe* recipe;
                pkgupd::package* pkg;
                
                /*
                 * check if package path is provided
                 */
                if (std::filesystem::exists(pkgid) &&
                    std::filesystem::path(pkgid).extension() != ".yml")
                {
                    io::info("found ",color::MAGENTA,"'",pkgid,"'", color::RESET, color::BOLD, " as ", color::CYAN, "package");
                    auto plug = installer.get_plugin(pkgid);
                    if (plug == nullptr)
                    {
                        io::error(installer.error());
                        return 1;
                    }

                    auto [_recipe,_pkg] = plug->get(pkgid);
                    if (_recipe == nullptr)
                    {
                        io::error(plug->error());
                        return 1;
                    }

                    pkgid = database.pkgid(recipe->id(), (pkg == nullptr ? "" : pkg->id()));
                    if (database.installed(pkgid) && !cc.checkflag("force"))
                    {
                        io::info(color::MAGENTA, "'", pkgid, "'", color::RESET, color::BOLD, " is already installed");
                        return 1;
                    }

                    // setup data
                    packages.push_back(pkgid);
                    recipe = _recipe;
                    pkg = _pkg;
                    
                }

                /*
                 * else assume it as recipe
                 */
                else
                {
                    // io::process("downloading ", color::MAGENTA, "'", pkgid,"'");

                    if (database.installed(pkgid) && !cc.checkflag("force"))
                    {
                        io::info(color::MAGENTA, "'", pkgid, "'", color::RESET, color::BOLD, " is already installed");
                        return 1;
                    }

                    auto [_recipe_id, _subpkg_id] = database.parse_pkgid(pkgid);
                    recipe = new pkgupd::recipe(database[_recipe_id]);
                    pkg = (*recipe)[_subpkg_id];

                    if (cc.checkflag("compile") || recipe->compile())
                    {
                        io::process("compiling ", pkgid);
                        if (!compiler.compile(recipe, pkg))
                        {
                            io::error(compiler.error());
                            return 0;
                        }
                        packages = compiler.packages();
                    } 
                    else
                    {
                        io::process("installing ", pkgid);
                        if (_subpkg_id.length() == 0)
                            packages = installer.download(*recipe);
                        else
                            packages = installer.download(*recipe, _subpkg_id);
                        
                        if (packages.size() == 0)
                        {
                            io::error(installer.error());
                            return 1;
                        }
                    }

                }

                for(auto const& i : packages)
                {
                    io::process("installing ", i);
                    if (!installer.install(i, !cc.checkflag("skip-script"), !cc.checkflag("skip-triggers"), !cc.checkflag("skip-triggers")))
                    {
                        io::error(installer.error());
                        return 1;
                    }
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
                    {
                        auto dep = database.resolve(pkg, cc.checkflag("all"));
                        dep.pop_back();
                        for(auto i : dep)
                            io::println(i);
                    }                            
                }
                catch(pkgupd::database::exception e)
                {
                    io::error(e.what());
                    return 1;
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
                    catch(rlx::obj::exception e)
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
                    auto pkg = cc.args()[0];
                    auto [pkgid, subpkg] = database.parse_pkgid(pkg);
                    auto recipe = database[pkgid];
                    io::println(
                        io::color::CYAN, "id", color::RESET, color::BOLD, "        :  ", recipe.id(),'\n',
                        io::color::CYAN, "version", color::RESET, color::BOLD, "   :  ", recipe.version(),'\n',
                        io::color::CYAN, "about", color::RESET, color::BOLD, "     :  ", recipe.about(),'\n',
                        io::color::CYAN, "provider", color::RESET, color::BOLD, "  :  ", recipe.pack(recipe[subpkg]),
                        io::color::RESET
                    );

                    if (database.installed(pkg))
                    {
                        auto installed_recipe = database.installed_recipe(pkg);
                    io::println(
                        io::color::CYAN, "installed", color::RESET, color::BOLD," :  ", color::GREEN, "True", '\n',
                        io::color::CYAN, "sys-ver", color::RESET, color::BOLD, "  : ", installed_recipe.version(), color::RESET
                    );
                    } else
                    {
                    io::println(
                        io::color::CYAN, "installed", color::RESET, color::BOLD," :  ", color::RED, "False", color::RESET
                    );
                    }
                    
                }
                catch(pkgupd::database::exception e)
                {
                    io::error(e.what());
                    return 1;
                }
                
                return 0;
            }))

        .sub(app::create("require")
            .about("Check which package require specified package")
            .fn([](context const& cc) -> int
            {
                if (cc.args().size() == 0)
                {
                    io::error("no pkgid file specified");
                    return 1;
                }

                string pkgid = cc.args()[0];
                io::debug(level::trace, "configuration:\n", cc.config());
                auto database = pkgupd::database(cc.config());

                for(auto const& i : database.repositories())
                {
                    string _repo_path = database.dir_recipe() + "/" + i;
                    for(auto const& j : std::filesystem::directory_iterator(_repo_path))
                    {
                        if (j.path().extension() != ".yml") continue;
                        auto _recipe = pkgupd::recipe(j.path());

                        if (rlx::algo::contains(_recipe.runtime(), pkgid))
                            io::println("=> ", _recipe.id());
                    }
                }
                return 0;
                
            }))
    

        .sub(app::create("sync")
            .about("Synchronize local database with repositories")
            .fn([](context const& cc) -> int
            {
                auto tempfile = rlx::utils::sys::tempfile("/tmp", "rlx-recipes");
                auto database = pkgupd::database(cc.config());

                for(auto const& repo : database.repositories())
                {
                    auto rcp_dir = io::format(database.dir_recipe() ,"/",repo,"/");
                    io::process("syncing ", repo);
                    if (!database.get_from_server(repo,"", tempfile))
                    {
                        io::error(database.error());
                        return 1;
                    }

                    std::filesystem::remove_all(rcp_dir);
                    std::filesystem::create_directories(rcp_dir);

                    YAML::Node node;
                    try
                    {
                        node = YAML::LoadFile(tempfile);
                        // TODO add verifier
                    
                        for(auto const & i : node["recipes"])
                        {
                            
                            std::filesystem::create_directories(rcp_dir);
                            io::writefile(rcp_dir+i["id"].as<string>()+".yml", i);
                        }
                            

                        std::filesystem::remove(tempfile);
                    } 
                    catch (YAML::BadSubscript e)
                    {
                        io::error(e.what());
                        return 1;
                    }
                    
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
                        auto installer = pkgupd::installer(cc.config());
                        std::vector<string> packages;

                        for(auto const& i : outdated)
                        {
                            auto subcc = context(cc);
                            subcc.args({i});
                            subcc.flags({"force"});

                            int status;
                            if ((status = cc.exec("install", subcc)) != 0)
                            {
                                return status;
                            }
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