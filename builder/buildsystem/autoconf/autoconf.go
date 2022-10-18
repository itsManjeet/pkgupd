package autoconf

import (
	"fmt"
	"os"
	"os/exec"
	"path"
	"runtime"
	"strings"

	"github.com/itsmanjeet/pkgupd/recipe"
	"github.com/itsmanjeet/pkgupd/utils"
)

type AutoConf struct {
}

func (a AutoConf) Compile(mod *recipe.Module, srcdir string, destdir string) error {
	execute := func(bin string, args ...string) error {
		for i := range args {
			args[i] = os.ExpandEnv(args[i])
		}
		cmd := exec.Command(bin, args...)
		cmd.Dir = path.Join(srcdir, mod.Dir)
		if mod.BuildDir {
			cmd.Dir = path.Join(cmd.Dir, utils.BUILD_DIR)
		}
		cmd.Stderr = os.Stdout
		cmd.Stdin = os.Stdin
		cmd.Stdout = os.Stdout
		cmd.Env = os.Environ()
		cmd.Env = append(cmd.Env, "pkgupd_srcdir="+cmd.Dir)
		return cmd.Run()
	}

	if err := os.MkdirAll(path.Join(srcdir, mod.Dir, utils.BUILD_DIR), 0755); err != nil {
		return fmt.Errorf("failed to create build directory %s", err)
	}

	convert := func(data string, def []string, hotword string) []string {
		if len(data) == 0 {
			return def
		}
		userArgs := strings.Split(strings.Trim(data, " \n"), " ")
		if strings.Contains(data, hotword) {
			return userArgs
		}

		def = append(def, userArgs...)
		return def
	}

	configure := "./configure"
	if mod.BuildDir {
		configure = "../configure"
	}

	if err := execute(configure, convert(mod.Configure, []string{"--prefix=/usr", "--sysconfdir=/etc", "--libdir=/usr/lib", "--libexecdir=/usr/lib", "--sbindir=/usr/bin", "--bindir=/usr/bin", "--datadir=/usr/share", "--localstatedir=/var"}, "--prefix")...); err != nil {
		return fmt.Errorf("failed to configure %s", err)
	}

	if err := execute("make", convert(mod.Compile, []string{fmt.Sprintf("-j%d", runtime.NumCPU())}, "-C")...); err != nil {
		return fmt.Errorf("failed to compile %s", err)
	}

	if err := execute("make", convert(mod.Compile, []string{"install", "DESTDIR=" + os.Getenv("DESTDIR")}, "install")...); err != nil {
		return fmt.Errorf("failed to compile %s", err)
	}

	return nil
}
