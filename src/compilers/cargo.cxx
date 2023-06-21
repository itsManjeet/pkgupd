#include "cargo.hxx"

#include "../exec.hxx"
#include "../recipe.hxx"

namespace rlxos::libpkgupd {
    bool Cargo::compile(Recipe *recipe, Configuration *config, std::string dir,
                        std::string destdir, std::vector<std::string> &environ) {
        // Do build
        int status = Executor().execute("cargo build --release " + recipe->configure(),
                                        dir, environ);
        if (status != 0) {
            p_Error = "failed to build with cargo";
            return false;
        }

        // Do check
        status = Executor().execute("cargo test --release " + recipe->configure(), dir,
                                    environ);
        if (status != 0) {
            p_Error = "failed to do check with cargo";
            return false;
        }

        // do Install
        auto installArgs = recipe->install();
        if (installArgs.length() == 0) {
            installArgs = "--path .";
        }

        status = Executor().execute("cargo install --root=" + destdir + " --locked " +
                                    recipe->configure() + " " + installArgs,
                                    dir, environ);
        if (status != 0) {
            p_Error = "failed to do install with cargo";
            return false;
        }

        for (auto const &i: {".crates.toml", ".crates2.json"}) {
            if (std::filesystem::exists(destdir + "/usr/" + i)) {
                std::error_code err;
                std::filesystem::remove(destdir + "/usr/" + i, err);
            }
        }

        return true;
    }
}  // namespace rlxos::libpkgupd