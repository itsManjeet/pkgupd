package recipe

import "strings"

// Module holds the build information of specific package
type Module struct {
	BuildSystem  string   `yaml:"build-system" json:"build-system" xml:"build-system"`
	Dir          string   `yaml:"dir" json:"dir" xml:"dir"`
	BuildDir     bool     `yaml:"build-dir" json:"build-dir" xml:"build-dir"`
	Sources      []string `yaml:"sources" json:"sources" xml:"sources"`
	Configure    string   `yaml:"configure" json:"configure" xml:"configure"`
	Compile      string   `yaml:"compile" json:"compile" xml:"compile"`
	Install      string   `yaml:"install" json:"install" xml:"install"`
	PreScript    string   `yaml:"pre-script" json:"pre-script" xml:"pre-script"`
	Script       string   `yaml:"script" json:"script" xml:"script"`
	PostScript   string   `yaml:"post-script" json:"post-script" xml:"post-script"`
	SkipIfExists string   `yaml:"skip-if-exists"`
}

type SplitRule struct {
	Id   string   `yaml:"id"`
	Path []string `yaml:"path"`
}

// Recipe holds the build information of a package with all its dependencies involved
type Recipe struct {
	Id       string `yaml:"id" json:"id" xml:"id"`
	Version  string `yaml:"version" json:"version" xml:"version"`
	About    string `yaml:"about" json:"about" xml:"about"`
	SkipPack bool   `yaml:"skip-pack"`

	Environ    []string    `yaml:"environ"`
	Modules    []Module    `yaml:"modules" json:"modules" xml:"modules"`
	SplitRules []SplitRule `yaml:"split-rules"`
}

func (r *Recipe) Verify() error {
	for i := range r.Modules {
		r.Modules[i].Configure = strings.Trim(r.Modules[i].Configure, "\n")
		r.Modules[i].Compile = strings.Trim(r.Modules[i].Compile, "\n")
		r.Modules[i].Install = strings.Trim(r.Modules[i].Install, "\n")
	}
	return nil
}
