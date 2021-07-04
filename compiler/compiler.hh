#ifndef __COMPILER__
#define __COMPILER__

#include <rlx.hh>
#include <path/path.hh>
#include <curl/curl.hh>
#include <yaml-cpp/yaml.h>
#include <tuple>
#include "../recipe.hh"
#include "../config.hh"
#include "../installer/installer.hh"
#include "../database/database.hh"

namespace pkgupd
{
    using namespace rlx;
    using color = rlx::io::color;

    class compiler : public rlx::obj
    {
    public:
        typedef std::tuple<bool, string> (*build_func)(recipe const &, YAML::Node const &, package *, string dir, string wrkdir);
        typedef std::tuple<bool, string> (*pack_func)(recipe const &, YAML::Node const &, package *, string dir, string output);

    private:
        recipe _recipe;
        YAML::Node _config;
        database _database;

        string _dir_pkgs,
            _dir_src,
            _dir_work,
            _dir_data;

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
        compiler(recipe const &r,
                 YAML::Node const &c)
            : _recipe(r), _config(c),
              _database(c)
        {
            auto get_dir = [&](string path, string fallback) -> string
            {
                if (c["dir"] && c["dir"][path])
                    return c["dir"][path].as<string>();
                return fallback;
            };

            _dir_pkgs = get_dir("pkgs", DEFAULT_DIR_PKGS);
            _dir_src = get_dir("src", DEFAULT_DIR_SRC);
            _dir_work = get_dir("work", DEFAULT_DIR_WORK);
            _dir_data = get_dir("data", DEFAULT_DIR_DATA);

            _dir_pkgs += "/" + _database.getrepo(_recipe.id());

            _dir_work = rlx::utils::sys::tempdir(_dir_work, "pkgupd");

            _recipe.appendenviron("PKGUPD_PKGDIR=" + _dir_pkgs);
            _recipe.appendenviron("PKGUPD_SRCDIR=" + _dir_src);
            _recipe.appendenviron("PKGUPD_WORKDIR=" + _dir_work);
        }

        ~compiler()
        {
            clean();
        }

        void clean()
        {
            io::process("clearing cache");
            if (getenv("NO_CLEAN") == nullptr && std::filesystem::exists(_dir_work))
                std::filesystem::remove_all(_dir_work);
        }

        bool download(package *pkg)
        {
            auto srcs = pkg == nullptr ? _recipe.sources() : pkg->sources();

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

        bool prepare(package *pkg)
        {
            auto srcs = pkg == nullptr ? _recipe.sources() : pkg->sources();

            if (!std::filesystem::exists(_dir_work + "/src"))
                std::filesystem::create_directories(_dir_work + "/src");

            if (_recipe.port().length())
            {
                io::info("writing port file");
                io::writefile(_dir_work + "/src/" + _recipe.id(), _recipe.port());
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

        bool build(package *pkg)
        {
            assert(pkg != nullptr);

            io::process("building ", pkg->id());

            string src_dir = _dir_work + "/src/" + _recipe.dir(pkg);
            string pkg_dir = _dir_work + "/pkg/" + pkg->id();

            _recipe.appendenviron("pkgupd_srcdir=" + src_dir);
            _recipe.appendenviron("pkgupd_pkgdir=" + pkg_dir);

            if (!std::filesystem::exists(src_dir))
                std::filesystem::create_directories(src_dir);

            if (pkg->prescript().length())
            {
                io::process("executing prescript");
                if (utils::exec::command(pkg->prescript(), src_dir, _recipe.environ(pkg)))
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
                if (utils::exec::command(pkg->script(), src_dir, _recipe.environ(pkg)))
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
            build_func plugin_build_fn;
            try
            {
                plugin_build_fn = utils::dlmodule::load<build_func>(plugin_path, "pkgupd_build");
            }
            catch (std::runtime_error const &e)
            {
                _error = e.what();
                return false;
            }

            assert(plugin_build_fn != nullptr);

            io::process("executing build ", plugin);
            auto [status, output] = plugin_build_fn(_recipe, _config, pkg, src_dir, pkg_dir);
            if (!status)
            {
                _error = output;
                return false;
            }

            if (pkg->postscript().length())
            {
                io::process("executing postscript");
                if (utils::exec::command(pkg->postscript(), ".", _recipe.environ(pkg)))
                {
                    _error = "failed to execute postscript";
                    return false;
                }
            }

            if (!std::filesystem::exists(pkg_dir) && _recipe.pack(pkg) != "none")
            {
                _error = "no output generated";
                return false;
            }

            return true;
        }

        bool pack(package *pkg)
        {
            if (_recipe.pack(pkg) == "none")
            {
                path::writefile(_dir_data + "/" + _recipe.id() + ":" + pkg->id(), _recipe.node());
                return true;
            }

            string dir = _dir_work + "/pkg/" + pkg->id();

            if (_recipe.strip())
            {
                io::process("stripping output");

                std::string _filter = "cat";
                if (_recipe.no_strip().size())
                {
                    _filter = "grep -v ";
                    for (auto const &i : _recipe.no_strip())
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

                if (_recipe.strip_script().length() != 0)
                {
                    io::println("using custom strip script");
                    _script = _recipe.strip_script();
                }

                io::debug(level::trace, "striping in ", dir);
                if (WEXITSTATUS(system(("cd " + dir + "; " + _script).c_str())))
                {
                    _error = "failed to execute ";
                    return false;
                }
            }

            auto pack_id = _recipe.pack(pkg);

            io::info("found '", color::MAGENTA, pack_id, color::RESET, color::BOLD, "'");
            string plugin_path = utils::dlmodule::search(pack_id, "/lib/pkgupd/:/usr/lib/pkgupd", "PKGUPD_PLUGINS");
            if (plugin_path.length() == 0)
            {
                _error = "failed to find plugin " + pack_id;
                return false;
            }

            pack_func plugin_pack_func;
            try
            {
                plugin_pack_func = utils::dlmodule::load<pack_func>(plugin_path, "pkgupd_pack");
            }
            catch (std::runtime_error const &e)
            {
                _error = e.what();
                return false;
            }

            assert(plugin_pack_func != nullptr);

            auto package_path = _dir_pkgs + "/" + _recipe.id() + "-" + _recipe.version() + ":" + pkg->id() + "." + pack_id;

            auto [status, output] = plugin_pack_func(_recipe, _config, pkg, dir, package_path);
            if (!status)
            {
                _error = output;
                return false;
            }

            if (!std::filesystem::exists(package_path))
            {
                _error = "no output generated";
                return false;
            }

            if (_config["no-install"] && _config["no-install"].as<string>() == "1")
            {
                io::info("skipping installation");
            }
            else
            {
                auto [instlr, _pkg] = installer::frompath(package_path, _config);
                string subpkg = "";
                if (_pkg != nullptr)
                    subpkg = _pkg->id();
                if (!instlr.install(subpkg))
                {
                    _error = instlr.error();
                    return false;
                }
            }

            return true;
        }

        bool compile(string const &pkg_id = "")
        {

            for (auto const &d : {_dir_pkgs, _dir_work, _dir_src, _dir_data})
            {
                if (std::filesystem::exists(d))
                    continue;

                if (!std::filesystem::create_directories(d))
                {
                    _error = "failed to create " + d;
                    return false;
                }
            }
            if (!download(nullptr))
                return false;

            if (!prepare(nullptr))
                return false;

            if (_config["environ"])
            {
                for (auto const &i : _config["environ"])
                    _recipe.appendenviron(i.as<string>());
            }

            if (_recipe.prescript().length())
            {
                io::process("executing prescript");
                if (utils::exec::command(_recipe.prescript(), _dir_work, _recipe.environ()))
                {
                    _error = "failed to execute prescript";
                    return false;
                }
            }

            bool pkg_build = false;

            for (auto pkg : _recipe.packages())
            {
                if (pkg_id.length())
                    if (pkg_id != pkg.id())
                        continue;

                if (!download(&pkg))
                    return false;

                if (!prepare(&pkg))
                    return false;

                if (!build(&pkg))
                    return false;

                if (!pack(&pkg))
                    return false;

                pkg_build = true;

                if (!_recipe.clean())
                    io::info("skip cleaning");
                else
                    clean();
            }

            if (_recipe.postscript().length())
            {
                io::process("executing postscript");
                if (utils::exec::command(_recipe.postscript(), _dir_work, _recipe.environ()))
                {
                    _error = "failed to execute postscript";
                    return false;
                }
            }

            if (pkg_id.length() && !pkg_build)
            {
                _error = "no package found with id " + pkg_id;
                return false;
            }
            return true;
        }
    };
}

#endif