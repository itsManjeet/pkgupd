#include "../libpkgupd/libpkgupd.hh"
using namespace rlxos;

#include <iostream>
using namespace std;

int main(int argc, char** argv) {
  std::string recipefile = "./recipe.yml";
  std::string repo = "testing";

  if (argc > 1) {
    recipefile = argv[1];
  }
  if (argc > 2) {
    repo = argv[2];
  }

  YAML::Node recipeNode = YAML::LoadFile(recipefile);
  libpkgupd::Recipe recipe = libpkgupd::Recipe(recipeNode, recipefile, repo);

  std::string WORKDIR = "_pkgupd_workdir";
  if (getenv("WORKDIR")) {
    WORKDIR = getenv("WORKDIR");
  }

  libpkgupd::Builder builder = libpkgupd::Builder(
      WORKDIR + "/work", WORKDIR + "/sources", WORKDIR + "/pkgs");

  if (!builder.build(recipe, true)) {
    cerr << "Error! " << builder.error() << endl;
    return 1;
  }

  cout << "Build Success" << endl;

  return 0;
}