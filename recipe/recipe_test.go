package recipe

import (
	"reflect"
	"testing"

	"github.com/itsmanjeet/pkgupd/utils"
)

func TestRecipeLoadFromFile(t *testing.T) {
	recipe, err := utils.FromFile[Recipe]("sample.yml")
	if err != nil {
		t.Fatal(err)
	}
	recipe.Verify()

	original_recipe := &Recipe{
		Id:      "sample-project",
		Version: "2295",
		About:   "Sample project for testing",
		Modules: []Module{
			{
				Sources: []string{
					"https://url.com/first",
					"https://url.com/second",
				},
				BuildSystem: "autoconf",
				Configure:   "--prefix=/usr --sysconfdir=/etc",
				Install:     "DESTDIR=${pkgupd_pkgdir}",
			},
			{
				Sources: []string{
					"https://url2.com/first",
				},
				BuildSystem: "meson",
				Configure:   "-Denable_feature=false",
			},
		},
	}

	if !reflect.DeepEqual(recipe, original_recipe) {
		t.Fatalf("%v != %v", *recipe, *original_recipe)
	}
}
