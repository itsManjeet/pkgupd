/* 
 * Copyright (c) 2023 Manjeet Singh <itsmanjeet1998@gmail.com>.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "Builder.h"

#include <optional>
#include <regex>


#include "Downloader.h"
#include "ArchiveManager.h"
#include "Execute.h"

Builder::BuildInfo::BuildInfo(const std::string &filepath) {
    config.node["cache"] = "none";
    update_from_file(filepath);
    if (config.node["build-depends"]) {
        for (auto const &dep: config.node["build-depends"]) {
            auto depend = dep.as<std::string>();
            if (depend.ends_with(".yml")) depend = depend.substr(0, depend.length() - 4);
            build_time_depends.emplace_back(depend);
        };
    }
    if (config.node["sources"]) {
        for (auto const &dep: config.node["sources"]) sources.emplace_back(dep.as<std::string>());
    }
}

std::string Builder::BuildInfo::resolve(const std::string &data, const std::map<std::string, std::string> &variables) {
    std::regex pattern(R"(\%\{([^}]+)\})");
    std::smatch match;
    std::string result = data;

    while (std::regex_search(result, match, pattern)) {
        if (match.size() > 1) {
            std::string variable = match.str(1);
            auto it = variables.find(variable);
            if (it != variables.end()) {
                result.replace(match[0].first, match[0].second, it->second);
            } else {
                throw std::runtime_error("undefined variable '" + variable + "'");
            }
        }
    }
    return result;
}

void Builder::BuildInfo::resolve(const Configuration &global) {
    std::map<std::string, std::string> variables;
    if (global.node["variables"]) {
        for (auto const &v: global.node["variables"]) {
            variables[v.first.as<std::string>()] = v.second.as<std::string>();
        }
    }

    if (this->config.node["variables"]) {
        for (auto const &v: this->config.node["variables"]) {
            variables[v.first.as<std::string>()] = v.second.as<std::string>();
        }
    }


    variables["version"] = version;
    variables["id"] = id;

    for (auto &source: sources) {
        source = resolve(source, variables);
    }
}


std::optional<std::filesystem::path>
Builder::prepare_sources(const std::filesystem::path &source_dir, const std::filesystem::path &build_root) {
    std::optional<std::filesystem::path> subdir;

    for (auto url: build_info.sources) {
        auto filename = std::filesystem::path(url).filename().string();
        if (auto idx = url.find("::"); idx != std::string::npos) {
            filename = url.substr(0, idx);
            url = url.substr(idx + 2);
        }

        auto filepath = source_dir / filename;
        if (!std::filesystem::exists(filepath)) {
            if (url.starts_with("http")) {
                if (int status = Executor("/bin/wget")
                            .arg(url)
                            .arg("-O")
                            .arg(filepath)
                            .start()
                            .wait(&std::cout); status != 0) {
                    throw std::runtime_error("failed to run wget " + std::to_string(status));
                }
            }

        }
        if (ArchiveManager::is_archive(filepath)) {
            std::vector<std::string> files_list;
            ArchiveManager::extract(filepath, build_root, files_list);
            if (!subdir) {
                subdir = files_list.front();
            }
        } else {
            std::filesystem::copy_file(filepath, build_root / filename);
        }
    }
    return subdir;
}

void Builder::compile_source(const std::filesystem::path &build_root, const std::filesystem::path &install_root) {
    auto script = build_info.config.get<std::string>("script", "");
    if (script.empty()) {
        auto compiler = get_compiler(build_root);
        script = compiler.script;
    }

    std::vector<std::string> environ;
    if (config.node["environ"]) {
        for (auto const &env: config.node["environ"]) {
            environ.push_back(env.as<std::string>());
        }
    }

    if (build_info.config.node["environ"]) {
        for (auto const &env: build_info.config.node["environ"]) {
            environ.push_back(env.as<std::string>());
        }
    }

    std::map<std::string, std::string> variables;
    if (config.node["variables"]) {
        for (auto const &v: config.node["variables"]) {
            variables[v.first.as<std::string>()] = v.second.as<std::string>();
        }
    }

    if (build_info.config.node["variables"]) {
        for (auto const &v: build_info.config.node["variables"]) {
            variables[v.first.as<std::string>()] = v.second.as<std::string>();
        }
    }

    variables["install-root"] = install_root.string();
    variables["build-root"] = build_root.string();
    script = BuildInfo::resolve(script, variables);

    PROCESS("Executing compilation script")
    DEBUG(script);

    auto status = Executor("/bin/sh")
            .arg("-ec")
            .arg(script)
            .path(build_root)
            .environ(environ)
            .start()
            .wait(&std::cout);
    if (status != 0) {
        throw std::runtime_error("failed to execute compiler script: exit code " + std::to_string(status));
    }
}

void Builder::pack(const std::filesystem::path &install_root, const std::filesystem::path &package) {
    for (auto const &i: std::filesystem::recursive_directory_iterator(install_root)) {
        if (i.is_directory()) {
            if (i.path().empty()) {
                DEBUG("CLEANUP " << i.path());
                std::filesystem::remove(i.path());
            }
        } else if (i.path().has_extension() && i.path().extension() == ".la") {
            DEBUG("CLEANUP " << i.path());
            std::filesystem::remove(i.path());
        }
    }
    ArchiveManager::compress(package, install_root);
}

Builder::Compiler Builder::get_compiler(const std::filesystem::path &build_root) {
    std::string build_type;
    if (config.node["build-type"]) {
        build_type = config.node["build-type"].as<std::string>();
    } else {
        for (auto const &[id, compiler]: compilers) {
            if (std::filesystem::exists(build_root / compiler.file)) {
                build_type = id;
                break;
            }
        }
    }

    if (build_type.empty() || !compilers.contains(build_type)) {
        throw std::runtime_error(
                "unknown build-type or failed to detect build-type '" + build_type + "' at " + build_root.string());
    }
    return compilers[build_type];
}

Builder::Builder(
        const Configuration &config,
        const Builder::BuildInfo &build_info)
        : config{config}, build_info{build_info} {
    if (config.node["compiler"]) {
        for (auto const &c: config.node["compiler"]) {
            compilers[c.first.as<std::string>()] = Compiler{
                    c.second["file"].as<std::string>(),
                    c.second["script"].as<std::string>(),
            };
        }
    }
}

