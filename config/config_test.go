package config

import (
	"reflect"
	"testing"

	"github.com/itsmanjeet/pkgupd/utils"
)

func TestConfigFromFile(t *testing.T) {
	config, err := utils.FromFile[Configuration]("sample.yml")
	if err != nil {
		t.Fatal(err)
	}

	origConfig := &Configuration{
		Environ: []string{"PATH=${PKGUPD_PKGDIR}/PATH"},
	}

	if !reflect.DeepEqual(*config, *origConfig) {
		t.Fatalf("%v != %v", *config, *origConfig)
	}
}
