#include "go.hxx"

#include "../exec.hxx"
#include "../recipe.hxx"

namespace rlxos::libpkgupd {
    bool Go::compile(Recipe *recipe, Configuration *config, std::string dir,
                     std::string destdir, std::vector<std::string> &environ) {
        environ.push_back("CGO_CFLAGS=${CFLAGS:-''}");
        environ.push_back("CGO_CPPFLAGS=${CPPFLAGS:-''}");
        environ.push_back("CGO_CXXFLAGS=${CXXFLAGS:-''}");
        environ.push_back("CGO_LDFLAGS=${CGO_LDFLAGS:-''}");
        environ.push_back("GO_BUILD_TAGS=${GO_BUILD_TAGS:-''}");
        environ.push_back("GO_LDFLAGS=${GO_LDFLAGS:-''}");
        environ.push_back("MAKEJOBS=${MAKEJOBS:-1}");
        environ.push_back("CGO_ENABLED=1");
        environ.push_back("GO111MODULE=auto");
        environ.push_back("GOPATH=${pkgupd_srcdir}");

        // Do configure
        if (!recipe->node()["GoImportPath"]) {
            p_Error = "GoImportPath not set on " + recipe->id();
            return false;
        }
        auto goImportPath = recipe->node()["GoImportPath"].as<std::string>();
        environ.push_back("GOSRCPATH=${GOPATH}/src/" + goImportPath);
        // Do Build
        auto goPackage = goImportPath;
        if (recipe->node()["GoPackage"]) {
            goPackage = recipe->node()["GoPackage"].as<std::string>();
        }

        std::string goModMode = "";
        if (recipe->node()["GoMode"]) {
            goModMode = recipe->node()["GoMode"].as<std::string>();
        }

        if (goModMode != "off" && std::filesystem::exists(dir + "/go.mod")) {
            if (goModMode.length() == 0 && std::filesystem::exists(dir + "/vendor")) {
                INFO("using vendor dir for " << recipe->id() << " go dependencies");
                goModMode = "vendor";
            } else if (goModMode == "default") {
                goModMode.clear();
            }

            if (goModMode.length()) {
                goModMode = "-mode=\"" + goModMode + "\"";
            }

            int status = Executor().execute(
                    "go install -p ${MAKEJOBS} " + goModMode +
                    " -x -tags \"${GO_BUILD_TAGS}\" -ldflags \"${GO_LDFLAGS}\" " +
                    goPackage,
                    dir, environ);
            if (status != 0) {
                p_Error = "failed to build using Go module";
                return false;
            }
        } else {
            int status = Executor().execute(
                    "go install -p ${MAKEJOBS} -tags \"${GO_BUILD_TAGS}\" -ldflags "
                    "\"${GO_LDFLAGS}\" " +
                    goPackage,
                    dir, environ);
            if (status != 0) {
                p_Error = "failed to build using Go module";
                return false;
            }
        }

        auto status = Executor().execute(
                "install -v -D -m 0755 ${GOPATH}/bin/* -t "
                "${pkgupd_pkgdir}/usr/bin/",
                dir, environ);
        if (status != 0) {
            p_Error = "failed to install go binaries";
            return false;
        }

        return true;
    }
}  // namespace rlxos::libpkgupd