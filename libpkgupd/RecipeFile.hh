#ifndef _PKGUPD_PKGINFO_HH_
#define _PKGUPD_PKGINFO_HH_

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include "PackageInfo.hh"
namespace rlxos::libpkgupd
{
    class User
    {
    private:
        unsigned int id;
        std::string name;
        std::string about;
        std::string dir;
        std::string shell;
        std::string group;

    public:
        User(unsigned int id,
             std::string const &name,
             std::string const &about,
             std::string const &dir,
             std::string const &shell,
             std::string const &group)
            : id(id),
              name(name),
              about(about),
              dir(dir),
              shell(shell),
              group(group)
        {
        }

        User(YAML::Node const &data, std::string const &file);

        bool IsExist() const;

        std::vector<std::string> GetCommandArguments() const;
    };

    class Group
    {
    private:
        unsigned int id;
        std::string name;

    public:
        Group(unsigned int id,
              std::string const &name)
            : id(id),
              name(name) {}

        Group(YAML::Node const &data, std::string const &file);

        bool IsExist() const;

        std::vector<std::string> GetCommandArguments() const;
    };

    class Flag
    {
    private:
        std::string id;
        std::string value;

        bool force;

    public:
        Flag(YAML::Node const &data, std::string const &file);

        std::string const &ID() const
        {
            return id;
        }

        std::string const &Value() const
        {
            return value;
        }

        bool Force() const
        {
            return force;
        }
    };

    class RecipeFile;

    class RecipePackageInfo : public PackageInfo
    {
    private:
        std::string id;

        std::string dir;
        bool nostrip;

        std::string pack;

        std::vector<std::string> skipstrip;

        std::vector<std::string> runtimeDepends;
        std::vector<std::string> buildtimeDepends;

        std::vector<std::string> sources;

        std::vector<std::string> environ;

        std::string prescript, postscript;
        std::string preinstall, postinstall;

        std::vector<std::shared_ptr<Flag>> flags;

        std::shared_ptr<RecipeFile> parentRecipeFile;

        std::string script;

    public:
        RecipePackageInfo(YAML::Node const &data, std::string const &file);

        void SetParent(std::shared_ptr<RecipeFile> parent)
        {
            parentRecipeFile = parent;
        }

        std::string ID() const;

        std::string const &OriginalID() const
        {
            return id;
        }

        std::string Version() const;

        std::string About() const;

        std::vector<std::string> Sources() const;

        std::string const &Script() const
        {
            return script;
        }

        std::string const &PreScript() const
        {
            return prescript;
        }

        std::string const &PostScript() const
        {
            return postscript;
        }

        std::vector<std::shared_ptr<Flag>> Flags() const
        {
            return flags;
        }

        std::string const &Pack() const
        {
            return pack;
        }

        std::string Dir() const
        {
            return dir;
        }

        std::vector<std::string> const &SkipStrip() const
        {
            return skipstrip;
        }

        bool NoStrip() const
        {
            return nostrip;
        }

        std::vector<std::string> Environ();

        void AddEnviron(std::string const &env)
        {
            environ.push_back(env);
        }

        std::vector<std::string> Depends(bool all) const;
    };

    class RecipeFile : public std::enable_shared_from_this<RecipeFile>
    {
    private:
        std::string id;
        std::string version;
        std::string about;

        std::vector<std::string> runtimeDepends;
        std::vector<std::string> buildtimeDepends;

        std::vector<std::string> sources;

        std::vector<std::string> environ;

        std::vector<std::shared_ptr<User>> users;
        std::vector<std::shared_ptr<Group>> groups;

        std::vector<std::shared_ptr<RecipePackageInfo>> packages;

    public:
        RecipeFile(YAML::Node const &node, std::string const &file);

        static std::shared_ptr<RecipeFile> FromFilePath(std::string const &filepath)
        {
            auto ptr = std::make_shared<RecipeFile>(YAML::LoadFile(filepath), filepath);
            for (auto &pkg : ptr->packages)
                pkg->SetParent(ptr->shared_from_this());

            return ptr;
        }

        static std::shared_ptr<RecipeFile> FromNode(YAML::Node const &node)
        {
            auto ptr = std::make_shared<RecipeFile>(node, "");
            for (auto &pkg : ptr->packages)
                pkg->SetParent(ptr->shared_from_this());

            return ptr;
        }

        static std::shared_ptr<RecipeFile> FromPackage(std::string const &pkgpath);

        std::shared_ptr<RecipePackageInfo> operator[](std::string const &pkgid) const;

        std::vector<std::shared_ptr<RecipePackageInfo>> Get() const
        {
            return packages;
        }

        std::string const &ID() const
        {
            return id;
        }

        friend class RecipePackageInfo;
    };

}

#endif
