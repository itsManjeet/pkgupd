package main

import (
	"log"
	"os"

	"github.com/itsmanjeet/pkgupd/builder"
	"github.com/itsmanjeet/pkgupd/config"
	"github.com/itsmanjeet/pkgupd/recipe"
	"github.com/itsmanjeet/pkgupd/utils"
)

func main() {
	recipe, err := utils.FromFile[recipe.Recipe](os.Args[1])
	if err != nil {
		log.Fatal(err)
	}

	config := config.Configuration{
		Cachedir: "/tmp/pkgupd",
	}
	config.Verify()

	recipeBuilder, _ := builder.New(&config)

	if err := recipeBuilder.Build(recipe); err != nil {
		log.Fatal(err)
	}
}
