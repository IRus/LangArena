package main

import (
	"os"
	"LangArena/LangArena"
)

func main() {
	if len(os.Args) > 1 {
		LangArena.LoadConfig(os.Args[1])
	} else {
		LangArena.LoadConfig("../run.js")
	}

	if len(os.Args) > 2 {
		LangArena.RunBenchmarks(os.Args[2])
	} else {
		LangArena.RunBenchmarks("")
	}
}
