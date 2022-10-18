package config

import (
	"os"
	"path"
)

// Configuration holds the various user configurations to manipulate PKGUPD
// behaviour
type Configuration struct {
	Environ  []string `yaml:"environ" json:"environ" xml:"environ"`
	Tempdir  string   `yaml:"dir.temp" json:"dir.temp" xml:"dir.temp"`
	Cachedir string   `yaml:"dir.cache" json:"dir.cache" xml:"dir.cache"`
}

// Verify the configuration file and fill up must have values
func (c *Configuration) Verify() error {
	if len(c.Tempdir) == 0 {
		c.Tempdir = os.TempDir()
	}
	if len(c.Cachedir) == 0 {
		c.Cachedir = path.Join("/", "var", "cache", "pkgupd")
	}
	return nil
}
