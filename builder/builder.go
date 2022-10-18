package builder

import (
	"fmt"
	"io/fs"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"regexp"
	"strings"

	"github.com/itsmanjeet/pkgupd/builder/buildsystem"
	"github.com/itsmanjeet/pkgupd/builder/buildsystem/autoconf"
	"github.com/itsmanjeet/pkgupd/builder/buildsystem/meson"
	"github.com/itsmanjeet/pkgupd/config"
	"github.com/itsmanjeet/pkgupd/recipe"
	"github.com/itsmanjeet/pkgupd/utils"
)

// Builder build the binary package out of recipe file
type Builder struct {
	Config       *config.Configuration
	workdir      string
	pkgdir       string
	srcdir       string
	_PKGDIR      string
	_SRCDIR      string
	BuildSystems map[string]buildsystem.BuildSystem
}

func New(conf *config.Configuration) (*Builder, error) {
	builder := &Builder{
		Config: conf,
		BuildSystems: map[string]buildsystem.BuildSystem{
			"autoconf": autoconf.AutoConf{},
			"meson":    meson.Meson{},
		},
	}
	return builder, nil
}

func (b *Builder) Setup(recipe *recipe.Recipe) (err error) {
	b.workdir, err = ioutil.TempDir(b.Config.Tempdir, "builder-*")
	if err != nil {
		return
	}

	b.pkgdir = path.Join(b.workdir, "pkg")
	b.srcdir = path.Join(b.workdir, "src")
	b._PKGDIR = path.Join(b.Config.Cachedir, "pkgs")
	b._SRCDIR = path.Join(b.Config.Cachedir, "src")

	os.Setenv("pkgupd_pkgdir", b.pkgdir)

	os.Setenv("PKGUPD_PKGDIR", b._PKGDIR)
	os.Setenv("PKGUPD_SRCDIR", b._SRCDIR)
	os.Setenv("DESTDIR", b.pkgdir)

	for _, env := range b.Config.Environ {
		data := strings.Split(env, "=")
		os.Setenv(data[0], os.ExpandEnv(data[1]))
	}

	for _, env := range recipe.Environ {
		data := strings.Split(env, "=")
		os.Setenv(data[0], os.ExpandEnv(data[1]))
	}

	for key, val := range map[string]string{"PATH": "usr/bin", "LD_LIBRARY_PATH": "usr/lib",
		"C_INCLUDE_DIR": "usr/include", "CPP_INCLUDE_DIR": "usr/include",
		"PKG_CONFIG_PATH": "usr/lib/pkgconfig:usr/share/pkgconfig"} {
		extra := os.Getenv(key)
		if len(extra) != 0 {
			extra = ":" + extra
		}
		os.Setenv(key, os.ExpandEnv(path.Join(b.pkgdir, val)+extra))
	}

	for _, dir := range []string{b.pkgdir, b.srcdir, b._PKGDIR, b._SRCDIR} {
		if err := os.MkdirAll(dir, 0755); err != nil {
			return err
		}
	}

	return
}

func (b *Builder) Cleanup() error {
	if len(os.Getenv("PKGUPD_NO_CLEANUP")) == 0 {
		os.RemoveAll(b.workdir)
	}
	return nil
}

func (b *Builder) Build(recipe *recipe.Recipe) error {
	if err := b.Setup(recipe); err != nil {
		return fmt.Errorf("failed to setup build %v", err)
	}
	defer b.Cleanup()

	fmt.Println("Environment", os.Environ())

	for i, module := range recipe.Modules {
		log.Printf("building %d th module\n", i)
		if err := b.BuildModule(&module); err != nil {
			return fmt.Errorf("failed to build %d th module, %s", i, err)
		}
	}
	if !recipe.SkipPack {
		log.Printf("packing %s\n", b.pkgdir)
		if err := b.Pack(recipe, b.pkgdir); err != nil {
			return fmt.Errorf("failed to pack %s, %s", b.pkgdir, err)
		}
	}
	return nil
}

func (b *Builder) Pack(recipe *recipe.Recipe, pkgdir string) error {
	for _, rule := range recipe.SplitRules {
		log.Println("splitting", rule.Id)
		splitDir := pkgdir + "_" + rule.Id
		if err := os.MkdirAll(splitDir, 0755); err != nil {
			return fmt.Errorf("failed to create split dir %s, %s", splitDir, err)
		}

		filepath.WalkDir(pkgdir, func(p string, d fs.DirEntry, err error) error {
			for _, reg := range rule.Path {
				if s, _ := regexp.MatchString(reg, p); s {
					dir := path.Join(splitDir, strings.TrimPrefix(path.Dir(p), pkgdir))
					if err := os.MkdirAll(dir, 0755); err != nil {
						return fmt.Errorf("failed to build required split dir %s, %s", dir, err)
					}
					if err := os.Rename(p, path.Join(dir, path.Base(p))); err != nil {
						return fmt.Errorf("failed to move %s into split dir %s, %s", p, splitDir, err)
					}
					break
				}
			}
			return err
		})

		output, err := exec.Command("tar", "--zstd", "-caf", fmt.Sprintf("%s/%s-%s-%s.tar.zst", b._PKGDIR, recipe.Id, rule.Id, recipe.Version), "-C", splitDir, ".").CombinedOutput()
		if err != nil {
			return fmt.Errorf("failed to pack %s, %s %s", recipe.Id, output, err)
		}
	}
	output, err := exec.Command("tar", "--zstd", "-caf", fmt.Sprintf("%s/%s-%s.tar.zst", b._PKGDIR, recipe.Id, recipe.Version), "-C", pkgdir, ".").CombinedOutput()
	if err != nil {
		return fmt.Errorf("failed to pack %s, %s %s", recipe.Id, output, err)
	}
	return nil
}

func (b *Builder) PrepareModuleSources(mod *recipe.Module, sourcesDir, srcDir string) error {
	for _, source := range mod.Sources {
		outputfile := path.Join(sourcesDir, path.Base(source))
		log.Printf("downloading %s at %s\n", source, outputfile)
		if err := utils.DownloadFile(outputfile, source); err != nil {
			return fmt.Errorf("failed to download %s, %s", source, err)
		}
		if utils.IsTar(outputfile) {
			log.Printf("extracting %s at %s\n", outputfile, srcDir)
			if output, err := exec.Command("bsdtar", "-xf", outputfile, "-C", srcDir).CombinedOutput(); err != nil {
				return fmt.Errorf("failed to extract %s, %s %s", outputfile, output, err)
			}
		} else {
			destfile := path.Join(srcDir, path.Base(source))
			log.Printf("copying %s at %s\n", outputfile, destfile)
			if err := utils.CopyFile(outputfile, destfile); err != nil {
				return fmt.Errorf("failed to copy file %s -> %s, %s", outputfile, destfile, err)
			}
		}
	}
	return nil
}

func (b *Builder) BuildModule(mod *recipe.Module) error {
	if len(mod.SkipIfExists) != 0 {
		if _, err := os.Stat(os.ExpandEnv(mod.SkipIfExists)); err == nil {
			log.Println("Skipping module")
			return nil
		}
	}
	log.Println("preparing module sources")
	if err := b.PrepareModuleSources(mod, b._SRCDIR, b.srcdir); err != nil {
		return err
	}

	execute := func(bin string, args ...string) error {
		cmd := exec.Command(bin, args...)
		cmd.Dir = path.Join(b.srcdir, mod.Dir)
		cmd.Stderr = os.Stderr
		cmd.Stdin = os.Stdin
		cmd.Stdout = os.Stdout
		cmd.Env = os.Environ()
		cmd.Env = append(cmd.Env, "pkgupd_srcdir="+cmd.Dir)
		return cmd.Run()
	}

	log.Println("executing pre-script")
	if err := execute("bash", "-c", mod.PreScript); err != nil {
		return fmt.Errorf("failed to execute pre execution script, with error: %v, check log for more information", err)
	}

	if len(mod.Script) != 0 {
		log.Println("executing script")
		if err := execute("bash", "-c", mod.Script); err != nil {
			return fmt.Errorf("failed to execute execution script, with error: %v, check log for more information", err)
		}
	} else {
		buildSystem, ok := b.BuildSystems[mod.BuildSystem]
		if !ok {
			return fmt.Errorf("unknown build system used %s", mod.BuildSystem)
		}

		log.Println("compiling source code using", mod.BuildSystem)
		if err := buildSystem.Compile(mod, b.srcdir, b.pkgdir); err != nil {
			return err
		}
	}

	log.Println("executing post-script")
	if err := execute("bash", "-c", mod.PostScript); err != nil {
		return fmt.Errorf("failed to execute post execution script, with error: %v, check log for more information", err)
	}
	return nil
}
