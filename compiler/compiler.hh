#ifndef __COMPILER__
#define __COMPILER__

#include <rlx.hh>
#include <path/path.hh>
#include <curl/curl.hh>
#include <yaml-cpp/yaml.h>
#include <tuple>
#include "../recipe.hh"
#include "../config.hh"
#include "plugin.hh"
#include "../installer/installer.hh"
#include "../database/database.hh"

namespace pkgupd
{
    using namespace rlx;
    using color = rlx::io::color;

    class compiler : public rlx::obj
    {
    public:
        using plugin_init = plugin::compiler *(*)(YAML::Node const &);

    private:
        YAML::Node _config;
        database _database;

        std::vector<string> _packages;

        std::tuple<string, string> parse_url(string const &u)
        {
            string url = u;
            string name = path::basename(u);

            size_t idx = url.rfind("::");
            if (idx == string::npos)
                return {url, name};

            url = u.substr(idx + 2, u.length() - (idx + 2));
            name = u.substr(0, idx);

            return {url, name};
        }

    public:
        compiler(YAML::Node const &c)
            : _config(c),
              _database(c)
        {
            auto get_dir = [&](string path, string fallback) -> string
            {
                if (c["dir"] && c["dir"][path])
                    return c["dir"][path].as<string>();
                return fallback;
            };
        }

        ~compiler()
        {
        }

        DEFINE_GET_METHOD(std::vector<string>, packages);

        void clean(string dir)
        {
            io::process("clearing cache");
            if (getenv("NO_CLEAN") == nullptr && std::filesystem::exists(dir))
                std::filesystem::remove_all(dir);
        }

        bool download(recipe *_recipe, package *pkg, string _dir_src)
        {
            auto srcs = pkg == nullptr ? _recipe->sources() : pkg->sources();

            for (auto const &src : srcs)
            {
                auto [url, filename] = parse_url(src);

                auto pkgpath = _dir_src + "/" + filename;

                io::process("downloading ", filename, " from ", url);
                if (!rlx::curl::download(url, pkgpath))
                {
                    _error = "failed to download " + filename + " from " + url;
                    return false;
                }
            }

            return true;
        }

        bool prepare(recipe *_recipe, package *pkg, string _dir_src, string _dir_work)
        {
            auto srcs = pkg == nullptr ? _recipe->sources() : pkg->sources();

            if (!std::filesystem::exists(_dir_work + "/src"))
                std::filesystem::create_directories(_dir_work + "/src");

            if (_recipe->port().length() && pkg)
            {
                io::info("writing port file");
                io::writefile(_dir_work + "/src/" + pkg->id(), _recipe->port());
            }

            auto is_tar = [](string const &s) -> bool
            {
                for (auto i : {"xz", "gz", "bzip2", "zip", "bz2", "tgz", "txz", "tar"})
                {
                    if (path::has_ext(s, i))
                        return true;
                }
                return false;
            };

            for (auto const &src : srcs)
            {
                auto [url, filename] = parse_url(src);
                auto pkgpath = _dir_src + "/" + filename;

                if (is_tar(filename))
                {
                    io::process("extracting ", filename);
                    if (rlx::utils::exec::command("bsdtar -xf " + pkgpath + " -C " + _dir_work + "/src"))
                    {
                        _error = "failed to extract " + filename;
                        return false;
                    }
                }
                else
                {
                    io::process("copying ", filename);
                    if (rlx::utils::exec::command("cp " + pkgpath + " " + _dir_work + "/src"))
                    {
                        _error = "failed to copy " + filename;
                        return false;
                    }
                }
            }
            return true;
        }

        bool build(recipe *_recipe, package *pkg, string _dir_work, string _dir_pkg)
        {
            assert(pkg != nullptr);

            io::process("building ", pkg->id());

            string src_dir = _dir_work + "/src/" + _recipe->dir(pkg);
            string pkg_dir = _dir_work + "/pkg/" + pkg->id();

            _recipe->appendenviron("pkgupd_srcdir=" + src_dir);
            _recipe->appendenviron("pkgupd_pkgdir=" + pkg_dir);

            if (!std::filesystem::exists(src_dir))
                std::filesystem::create_directories(src_dir);

            if (pkg->prescript().length())
            {
                io::process("executing prescript");
                if (utils::exec::command(pkg->prescript(), src_dir, _recipe->environ(pkg)))
                {
                    _error = "failed to execute prescript";
                    return false;
                }
            }

            string plugin = pkg->plugin().length() == 0 ? "auto" : pkg->plugin();

            io::info("found '", color::MAGENTA, plugin, color::RESET, color::BOLD, "'");

            if (plugin == "script")
            {
                io::info("executing script");
                if (utils::exec::command(pkg->script(), src_dir, _recipe->environ(pkg)))
                {
                    _error = "failed to execute script";
                    return false;
                }
                return true;
            }
            else if (plugin == "skip")
            {
                return true;
            }

            auto plugin_path = utils::dlmodule::search(plugin, "/lib/pkgupd/:/usr/lib/pkgupd/", "PKGUPD_PLUGINS");
            if (plugin_path.length() == 0)
            {
                _error = "failed to find plugin '" + plugin + "' required to build " + pkg->id();
                return false;
            }

            auto plug_ = plugin_init();

            try
            {
                plug_ = utils::dlmodule::load<plugin_init>(plugin_path, "pkgupd_init");
            }
            catch (std::runtime_error const &e)
            {
                _error = e.what();
                return false;
            }

            assert(plug_ != nullptr);

            auto plug = plug_(_config);

            io::process("executing build ", plugin);
            if (!plug->compile(_recipe, pkg, src_dir, pkg_dir))
            {
                _error = plug->error();
                return false;
            }

            if (pkg->postscript().length())
            {
                io::process("executing postscript");
                if (utils::exec::command(pkg->postscript(), ".", _recipe->environ(pkg)))
                {
                    _error = "failed to execute postscript";
                    return false;
                }
            }

            if (!std::filesystem::exists(pkg_dir) && _recipe->pack(pkg) != "none")
            {
                _error = "no output generated";
                return false;
            }

            return true;
        }

        bool strip(recipe *_recipe, string dir)
        {
            std::string _filter = "cat";
            if (_recipe->no_strip().size())
            {
                _filter = "grep -v ";
                for (auto const &i : _recipe->no_strip())
                    _filter += " -e " + i.substr(0, i.length() - 1);
            }

            std::string _script =
                "find . -type f -printf \"%P\\n\" 2>/dev/null | " + _filter +
                " | while read -r binary ; do \n"
                " case \"$(file -bi \"$binary\")\" in \n"
                " *application/x-sharedlib*)      strip --strip-unneeded \"$binary\" ;; \n"
                " *application/x-pie-executable*) strip --strip-unneeded \"$binary\" ;; \n"
                " *application/x-archive*)        strip --strip-debug    \"$binary\" ;; \n"
                " *application/x-object*) \n"
                "    case \"$binary\" in \n"
                "     *.ko)                       strip --strip-unneeded \"$binary\" ;; \n"
                "     *)                          continue ;; \n"
                "    esac;; \n"
                " *application/x-executable*)     strip --strip-all \"$binary\" ;; \n"
                " *)                              continue ;; \n"
                " esac\n"
                " done\n";

            if (_recipe->strip_script().length() != 0)
            {
                io::println("using custom strip script");
                _script = _recipe->strip_script();
            }

            io::debug(level::trace, "striping in ", dir);
            if (WEXITSTATUS(system(("cd " + dir + "; " + _script).c_str())))
            {
                _error = "failed to execute ";
                return false;
            }

            return true;
        }

        bool pack(recipe *_recipe, package *pkg, string _dir_work, string _dir_pkgs)
        {
            if (_recipe->pack(pkg) == "none")
            {
                return true;
            }

            string dir = _dir_work + "/pkg/" + pkg->id();

            if (!std::filesystem::exists(dir))
            {
                _error = "No output folder generated";
                return false;
            }

            if (_recipe->strip())
            {
                io::process("stripping output");
                if (!strip(_recipe, dir))
                    return false;
            }

            auto pack_id = _recipe->pack(pkg);

            io::info("found '", color::MAGENTA, pack_id, color::RESET, color::BOLD, "'");

            auto _installer = installer(_config);
            auto _installer_plug_ = _installer.get_plugin(pack_id);

            auto package_path = _dir_pkgs + "/" + _recipe->id() + "-" + _recipe->version() + ":" + pkg->id() + "." + pack_id;

            if (!_installer_plug_->pack(*_recipe, pkg->id(), dir, package_path))
            {
                _error = _installer_plug_->error();
                return false;
            }

            if (!std::filesystem::exists(package_path))
            {
                _error = "no output generated";
                return false;
            }

            if (_recipe->node()["split"] && _recipe->node()["split"].as<bool>() == false)
            {
                if (!_installer.install(package_path))
                {
                    _error = _installer.error();
                    return false;
                }
            }

            _packages.push_back(package_path);

            return true;
        }

        bool compile(recipe *_recipe, package *_pkg)
        {
            assert(_recipe != nullptr);

            string _dir_work = rlx::utils::sys::tempdir(_database.dir_work(), "pkgupd");
            string _dir_pkg = _database.dir_pkgs() + "/" + _database.getrepo(_recipe->id());

            _recipe->appendenviron("PKGUPD_PKGDIR=" + _database.dir_pkgs() + "/" + _database.getrepo(_recipe->id()));
            _recipe->appendenviron("PKGUPD_SRCDIR=" + _database.dir_src());
            _recipe->appendenviron("PKGUPD_WORKDIR=" + _dir_work);

            if (!download(_recipe, nullptr, _database.dir_src()))
                return false;

            if (!prepare(_recipe, nullptr, _database.dir_src(), _dir_work))
                return false;

            if (_config["environ"])
            {
                for (auto const &i : _config["environ"])
                    _recipe->appendenviron(i.as<string>());
            }

            if (_recipe->prescript().length())
            {
                io::process("executing prescript");
                if (utils::exec::command(_recipe->prescript(), _dir_work, _recipe->environ()))
                {
                    _error = "failed to execute prescript";
                    return false;
                }
            }

            bool pkg_build = false;

            std::vector<package *> _pkgs;

            if (_pkg == nullptr)
                for (auto const &p : _recipe->packages())
                    _pkgs.push_back(new package(p));
            else
                _pkgs = {_pkg};

            for (auto p : _pkgs)
            {
                if (!download(_recipe, p, _database.dir_src()))
                    return false;

                if (!prepare(_recipe, p, _database.dir_src(), _dir_work))
                    return false;

                if (!build(_recipe, p, _dir_work, _dir_pkg))
                    return false;

                if (!pack(_recipe, p, _dir_work, _dir_pkg))
                    return false;

                if (_recipe->clean())
                    clean(_dir_work);
            }

            if (_recipe->postscript().length())
            {
                io::process("executing postscript");
                if (utils::exec::command(_recipe->postscript(), _dir_work, _recipe->environ()))
                {
                    _error = "failed to execute postscript";
                    return false;
                }
            }

            return true;
        }
    };
}

#endif