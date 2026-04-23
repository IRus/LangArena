package main

import (
	"os"
)

func main() {
	if len(os.Args) > 1 {
		LoadConfig(os.Args[1])
	} else {
		LoadConfig("../run.js")
	}

	if len(os.Args) > 2 {
		RunBenchmarks(os.Args[2])
	} else {
		RunBenchmarks("")
	}
}
