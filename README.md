# LangArena: A Balanced Programming Language Benchmark Suite

[Results Page](https://kostya.github.io/LangArena/)

## What is it?

A collection of 50 benchmarks for apples-to-apples comparisons across 22 languages. It focuses on production-like tasks (JSON, Base64, CSV, neural networks, compression, maze A*, graph algorithms, sorting, hashing, interpreters, parallel matmul, and more). Where possible, the same core algorithm is implemented across all languages using idiomatic constructs; for library-based (JSON, Base64, CSV), I use the best practical option for each language. Checksums verify correctness and prevent dead-code elimination. The suite runs monthly with full history, so you can watch compiler performance evolve over time. The goal is not to crown a micro-optimization champion, but to see how well each language's compiler or runtime optimizes clean, readable code.

**Contenders:** `C`, `C++`, `Crystal`, `Rust`, `Go`, `Swift`, `C#`, `Java`, `Kotlin`, `TypeScript`, `Zig`, `D`, `V`, `Julia`, `Nim`, `F#`, `Dart`, `Python`, `Odin`, `Scala`, `C3`, `Ruby`.

## Why?

Most existing benchmarks suffer from the same problems: they measure micro-optimizations or synthetic loops, mix multiple concepts so it's unclear what's being measured, and often degenerate into assembly or parallelism competitions rather than clean code. The same algorithm is rarely preserved across implementations. They measure code that nobody runs in production - where you use standard language constructs, not hand-tuned assembly - and they rely on unsafe flags (`--no-bounds-check`) that no one enables in real projects. On top of that, there's no history: I'd love to see how Clang improved over time on a simple graph, but most suites are one-off runs.

LangArena was built to fix all of this.

## Origin & Approach

The suite began with my original implementation in Crystal. AI tools assisted in translating it to other languages, but this was far from a one-shot process. Many implementations went through multiple iterations - fixing incorrect checksums, correcting algorithms, rewriting code I didn't like, or investigating unexpected slowness. Others were fine from the start. Either way, every implementation was reviewed for semantic correctness and idiomatic accuracy to ensure fair benchmarking.

**Handling Library Differences:**

One common objection to any multi-language benchmark is: "You picked the wrong library for language X!" To make this less of an issue, I introduced a Runtime Score - a normalized 0–100 metric based on four reference points: 100 (best result), 90 (average of the fastest languages), 50 (overall average), and 0 (worst result). This normalization is applied per benchmark, so each test contributes equally to the final score - regardless of its absolute runtime. The per-benchmark scores are then averaged across all 50 tests to produce the final Runtime Score for each language. This matters because it smooths out outliers. A language that's consistently fast but has one weak library (say, a bad JSON parser) won't be dragged down by a single test. And a language that's slow overall won't get a free pass just because it excelled in a couple of isolated cases. The result is a stable, holistic view of each language's real-world performance - not a ranking that hinges on a single library choice or a lucky benchmark.

## Core Philosophy

* Clean Code: Benchmarks are written in a clear, idiomatic style that prioritizes readability and maintainability.
* Algorithmic Consistency: The same core algorithm is used across all languages for each task.
* Standard vs Unsafe Modes: All benchmarks use standard production compiler flags (safe mode by default). A separate "Hacking" section explores unsafe optimizations - disabling bounds checks, runtime checks, and other trade-offs that prioritize speed over safety.
* Testing Language "Muscle": We measure the cost of abstractions. Can a language optimize clean, idiomatic code to efficient machine code? Languages that can (like Rust, Java) prove their compilers are powerful. Those that can't reveal the honest price of their abstractions. Benchmarks like matrix multiplication use naive implementations intentionally - not to measure C library calls (e.g., BLAS via numpy), but to see how efficiently the language handles fundamental computational patterns. Because one day, you'll have to write that loop yourself.
* Pull Requests Welcome: Improvements that maintain this philosophy and fix suboptimal implementations are encouraged.

## Benchmarking Methodology

Each benchmark runs in isolation, with data preparation excluded from timing. JIT-based languages (C#, Java, Julia, etc.) receive a separate warmup phase to reach steady-state performance before measurements begin. This ensures fair comparisons. Checksums verify algorithmic correctness across all implementations.

## Sources

Benchmark ideas were taken from:

*   The Computer Language Benchmarks Game
*   My own collections: [benchmarks](https://github.com/kostya/benchmarks), [jit-benchmarks](https://github.com/kostya/jit-benchmarks), [crystal-benchmarks-game](https://github.com/kostya/crystal-benchmarks-game), [crystal-metric](https://github.com/kostya/crystal-metric)
*   Crystal code samples

## Beyond Just Ranking

This suite is also a practical tool for:

* Compiler Tracking - monitor performance regressions and improvements across compiler versions.
* New Language Evaluation - get a standardized score to position a new language against established ones.

## Hardware

AMD Ryzen 7 3800X 8-Core, 78GB RAM (x86_64-linux-gnu)

# Running

## Local run without docker:

`cd Lang` and run scripts `./test` (for fast testing) and `./run` (for local measure):

	cd rust
	./test [BenchName]
	./run [BenchName]

## Run all benchmarks

Requires only: `docker`, `docker compose`, and `ruby`. Warning: Docker images take up more than 35 GB of disk space.

	sh build-docker.sh
	ruby benchmarks.rb

## Generate Website

	cd docs
	ruby gen.rb ../results/2026-02-02-x86_64-linux-gnu.js
	open index.html


