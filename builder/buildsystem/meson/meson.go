package meson

import (
	"fmt"
	"log"
	"os"
	"os/exec"
	"path"
	"runtime"
	"strings"

	"github.com/itsmanjeet/pkgupd/recipe"
	"github.com/itsmanjeet/pkgupd/utils"
)

type Meson struct {
}

func (m Meson) Compile(mod *recipe.Module, srcdir string, destdir string) error {
	execute := func(bin string, args ...string) error {
		log.Printf("Executing %s, %s\n", bin, args)
		cmd := exec.Command(bin, args...)
		cmd.Dir = path.Join(srcdir, mod.Dir)
		cmd.Stderr = os.Stderr
		cmd.Stdin = os.Stdin
		cmd.Stdout = os.Stdout
		cmd.Env = os.Environ()
		cmd.Env = append(cmd.Env, "pkgupd_srcdir="+cmd.Dir)
		return cmd.Run()
	}

	configure := "meson"
	configure_args := []string{"--prefix=/usr", "--sysconfdir=/etc", "--libdir=/usr/lib", "--libexecdir=/usr/lib", "--sbindir=/usr/bin", "--bindir=/usr/bin", "--datadir=/usr/share", "--localstatedir=/var", utils.BUILD_DIR}
	if strings.Contains(mod.Configure, "--prefix") {
		configure_args = []string{utils.BUILD_DIR}
	}
	configure_args = append(configure_args, strings.Split(strings.Trim(os.ExpandEnv(mod.Configure), " "), " ")...)

	if err := execute(configure, configure_args...); err != nil {
		return fmt.Errorf("failed to configure %s", err)
	}
	compile_args := []string{"-C", utils.BUILD_DIR, fmt.Sprintf("-j%d", runtime.NumCPU())}
	if len(mod.Compile) != 0 {
		compile_user_args := strings.Split(strings.Trim(os.ExpandEnv(mod.Compile), " "), " ")
		if len(compile_user_args) != 0 {
			compile_args = append(compile_args, compile_user_args...)
		}

	}

	fmt.Printf("compile_args: -%s-\n", strings.Join(compile_args, "::"))

	if err := execute("ninja", compile_args...); err != nil {
		return fmt.Errorf("failed to compile %s", err)
	}

	install_args := []string{"-C", utils.BUILD_DIR, "install"}
	if strings.Contains(mod.Install, "install") {
		install_args = []string{}
	}

	if len(mod.Install) != 0 {
		install_args = append(install_args, strings.Split(strings.Trim(os.ExpandEnv(mod.Install), " "), " ")...)
	}

	if err := execute("ninja", install_args...); err != nil {
		return fmt.Errorf("failed to install %s", err)
	}

	return nil
}
