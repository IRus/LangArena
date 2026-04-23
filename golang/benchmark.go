package main

import (
	"fmt"
	"os"
	"runtime"
	"strings"
	"time"
)

type Benchmark interface {
	Prepare()
	Run(int)
	Name() string
	Warmup(Benchmark)
	Checksum() uint32
	Iterations() int
	WarmupIterations() int
	ExpectedChecksum() int64
}

type BaseBenchmark struct {
	className string
}

func (b *BaseBenchmark) ConfigVal(field string) int64 {
	return configI64(b.className, field)
}

func (b *BaseBenchmark) ConfigStr(field string) string {
	return configS(b.className, field)
}

func (b *BaseBenchmark) WarmupIterations() int {
	if cfg, ok := CONFIG[b.className]; ok {
		if n, ok := cfg["warmup_iterations"]; ok {
			if n.IsInt {
				return int(n.IntValue)
			}
		}
	}

	iter := b.Iterations()
	warmup := int(float64(iter) * 0.2)
	if warmup < 1 {
		warmup = 1
	}
	return warmup
}

func Warmup(bench Benchmark) {
	bench.Warmup(bench)
}

func (b *BaseBenchmark) Prepare() {
}

func (b *BaseBenchmark) Name() string {
	return b.className
}

func (b *BaseBenchmark) Warmup(bench Benchmark) {
	wi := b.WarmupIterations()
	for i := 0; i < wi; i++ {
		bench.Run(i)
	}
}

func (b *BaseBenchmark) Run(iteration_id int) {
}

func (b *BaseBenchmark) Checksum() uint32 {
	return 0
}

func (b *BaseBenchmark) Iterations() int {
	return int(b.ConfigVal("iterations"))
}

func RunAll(bench Benchmark) {
	for i := 0; i < bench.Iterations(); i++ {
		bench.Run(i)
	}
}

func (b *BaseBenchmark) ExpectedChecksum() int64 {
	return b.ConfigVal("checksum")
}

func RunBenchmarks(singleBench string) {
	fmt.Printf("start: %d\n", time.Now().UnixMilli())

	summaryTime := 0.0
	ok_count := 0
	fails := 0

	singleBench = strings.ToLower(singleBench)

	benchMap := map[string]Benchmark{
		"Binarytrees::Obj":        &BinarytreesObj{BaseBenchmark: BaseBenchmark{className: "Binarytrees::Obj"}},
		"Binarytrees::Arena":      &BinarytreesArena{BaseBenchmark: BaseBenchmark{className: "Binarytrees::Arena"}},
		"Brainfuck::Array":        &BrainfuckArray{BaseBenchmark: BaseBenchmark{className: "Brainfuck::Array"}},
		"Brainfuck::Recursion":    &BrainfuckRecursion{BaseBenchmark: BaseBenchmark{className: "Brainfuck::Recursion"}},
		"CLBG::Fannkuchredux":     &Fannkuchredux{BaseBenchmark: BaseBenchmark{className: "CLBG::Fannkuchredux"}},
		"CLBG::Mandelbrot":        &Mandelbrot{BaseBenchmark: BaseBenchmark{className: "CLBG::Mandelbrot"}},
		"Matmul::Single":          &Matmul1T{BaseMatmul{BaseBenchmark: BaseBenchmark{className: "Matmul::Single"}}},
		"Matmul::T4":              &Matmul4T{BaseMatmul{BaseBenchmark: BaseBenchmark{className: "Matmul::T4"}}},
		"Matmul::T8":              &Matmul8T{BaseMatmul{BaseBenchmark: BaseBenchmark{className: "Matmul::T8"}}},
		"Matmul::T16":             &Matmul16T{BaseMatmul{BaseBenchmark: BaseBenchmark{className: "Matmul::T16"}}},
		"CLBG::Nbody":             &Nbody{BaseBenchmark: BaseBenchmark{className: "CLBG::Nbody"}},
		"CLBG::Spectralnorm":      &Spectralnorm{BaseBenchmark: BaseBenchmark{className: "CLBG::Spectralnorm"}},
		"Base64::Encode":          &Base64Encode{BaseBenchmark: BaseBenchmark{className: "Base64::Encode"}},
		"Base64::Decode":          &Base64Decode{BaseBenchmark: BaseBenchmark{className: "Base64::Decode"}},
		"Json::Generate":          &JsonGenerate{BaseBenchmark: BaseBenchmark{className: "Json::Generate"}},
		"Json::ParseDom":          &JsonParseDom{BaseBenchmark: BaseBenchmark{className: "Json::ParseDom"}},
		"Json::ParseMapping":      &JsonParseMapping{BaseBenchmark: BaseBenchmark{className: "Json::ParseMapping"}},
		"Etc::Sieve":              &Sieve{BaseBenchmark: BaseBenchmark{className: "Etc::Sieve"}},
		"Etc::TextRaytracer":      &TextRaytracer{BaseBenchmark: BaseBenchmark{className: "Etc::TextRaytracer"}},
		"Etc::NeuralNet":          &NeuralNet{BaseBenchmark: BaseBenchmark{className: "Etc::NeuralNet"}},
		"Sort::Quick":             &SortQuick{BaseBenchmark: BaseBenchmark{className: "Sort::Quick"}},
		"Sort::Merge":             &SortMerge{BaseBenchmark: BaseBenchmark{className: "Sort::Merge"}},
		"Sort::Self":              &SortSelf{BaseBenchmark: BaseBenchmark{className: "Sort::Self"}},
		"Graph::BFS":              &GraphPathBFS{BaseBenchmark: BaseBenchmark{className: "Graph::BFS"}},
		"Graph::DFS":              &GraphPathDFS{BaseBenchmark: BaseBenchmark{className: "Graph::DFS"}},
		"Graph::AStar":            &GraphPathAStar{BaseBenchmark: BaseBenchmark{className: "Graph::AStar"}},
		"Hash::SHA256":            &BufferHashSHA256{BaseBenchmark: BaseBenchmark{className: "Hash::SHA256"}},
		"Hash::CRC32":             &BufferHashCRC32{BaseBenchmark: BaseBenchmark{className: "Hash::CRC32"}},
		"Etc::CacheSimulation":    &CacheSimulation{BaseBenchmark: BaseBenchmark{className: "Etc::CacheSimulation"}},
		"Calculator::Ast":         &CalculatorAst{BaseBenchmark: BaseBenchmark{className: "Calculator::Ast"}},
		"Calculator::Interpreter": &CalculatorInterpreter{BaseBenchmark: BaseBenchmark{className: "Calculator::Interpreter"}},
		"Etc::GameOfLife":         &GameOfLife{BaseBenchmark: BaseBenchmark{className: "Etc::GameOfLife"}},
		"Maze::Generator":         &MazeGenerator{BaseBenchmark: BaseBenchmark{className: "Maze::Generator"}},
		"Maze::BFS":               &MazeBFS{BaseBenchmark: BaseBenchmark{className: "Maze::BFS"}},
		"Maze::AStar":             &MazeAStar{BaseBenchmark: BaseBenchmark{className: "Maze::AStar"}},
		"Compress::BWTEncode":     &BWTEncode{BaseBenchmark: BaseBenchmark{className: "Compress::BWTEncode"}},
		"Compress::BWTDecode":     &BWTDecode{BaseBenchmark: BaseBenchmark{className: "Compress::BWTDecode"}},
		"Compress::HuffEncode":    &HuffEncode{BaseBenchmark: BaseBenchmark{className: "Compress::HuffEncode"}},
		"Compress::HuffDecode":    &HuffDecode{BaseBenchmark: BaseBenchmark{className: "Compress::HuffDecode"}},
		"Compress::ArithEncode":   &ArithEncode{BaseBenchmark: BaseBenchmark{className: "Compress::ArithEncode"}},
		"Compress::ArithDecode":   &ArithDecode{BaseBenchmark: BaseBenchmark{className: "Compress::ArithDecode"}},
		"Compress::LZWEncode":     &LZWEncode{BaseBenchmark: BaseBenchmark{className: "Compress::LZWEncode"}},
		"Compress::LZWDecode":     &LZWDecode{BaseBenchmark: BaseBenchmark{className: "Compress::LZWDecode"}},
		"Distance::Jaro":          &Jaro{BaseBenchmark: BaseBenchmark{className: "Distance::Jaro"}},
		"Distance::NGram":         &NGram{BaseBenchmark: BaseBenchmark{className: "Distance::NGram"}},
		"Etc::Words":              &Words{BaseBenchmark: BaseBenchmark{className: "Etc::Words"}},
		"Etc::LogParser":          &LogParser{BaseBenchmark: BaseBenchmark{className: "Etc::LogParser"}},
		"Template::Regex":         &TemplateRegex{TemplateBase: TemplateBase{BaseBenchmark: BaseBenchmark{className: "Template::Regex"}}},
		"Template::Parse":         &TemplateParse{TemplateBase: TemplateBase{BaseBenchmark: BaseBenchmark{className: "Template::Parse"}}},
		"CSV::Parse":              &CsvParse{BaseBenchmark: BaseBenchmark{className: "CSV::Parse"}},
	}

	for _, className := range ORDER {
		if singleBench != "" && !strings.Contains(strings.ToLower(className), singleBench) {
			continue
		}

		bench, ok := benchMap[className]
		if !ok {
			fmt.Printf("Warning: Benchmark '%s' defined in config but not found in code\n", className)
			continue
		}

		fmt.Printf("%s: ", className)

		Reset()
		bench.Prepare()
		Warmup(bench)
		runtime.GC()

		Reset()

		start := time.Now()
		RunAll(bench)
		elapsed := time.Since(start).Seconds()

		chks := bench.Checksum()
		expected := uint32(bench.ExpectedChecksum())

		if chks == expected {
			fmt.Printf("OK ")
			ok_count++
		} else {
			fmt.Printf("ERR[actual=%d, expected=%d] ", chks, expected)
			fails++
		}

		runtime.GC()

		fmt.Printf("in %.3fs\n", elapsed)
		summaryTime += elapsed
	}

	fmt.Printf("Summary: %.4fs, %d, %d, %d\n", summaryTime, ok_count+fails, ok_count, fails)

	os.WriteFile("/tmp/recompile_marker", []byte("RECOMPILE_MARKER_0"), 0644)
	if fails > 0 {
		os.Exit(1)
	}
}
