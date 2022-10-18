package buildsystem

import (
	"github.com/itsmanjeet/pkgupd/recipe"
)

type BuildSystem interface {
	Compile(*recipe.Module, string, string) error
}
