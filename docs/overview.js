function overview_tab($results) {
    $results.append(`
<style>
.overview {
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
  line-height: 1.6;
  color: #1a1a1a;
  max-width: 1000px;
  margin: 0 auto;
  padding: 2rem 1.5rem;
  background-color: #ffffff;
}

.overview h1 {
  font-size: 2.2rem;
  font-weight: 500;
  color: #111111;
  margin-top: 0;
  margin-bottom: 1.5rem;
  padding-bottom: 0.5rem;
  border-bottom: 1px solid #cccccc;
  letter-spacing: -0.02em;
}

.overview h2 {
  font-size: 1.6rem;
  font-weight: 500;
  color: #222222;
  margin-top: 2rem;
  margin-bottom: 1rem;
  padding-bottom: 0.25rem;
  border-bottom: 1px solid #dddddd;
}

.overview h3 {
  font-size: 1.3rem;
  font-weight: 500;
  color: #333333;
  margin-top: 1.5rem;
  margin-bottom: 0.75rem;
}

.overview p {
  margin-bottom: 1.25rem;
  text-align: justify;
}

.overview strong {
  font-weight: 600;
  color: #000000;
}

.overview ul, ol {
  margin-top: 0.5rem;
  margin-bottom: 1.5rem;
  padding-left: 1.8rem;
}

.overview li {
  margin-bottom: 0.4rem;
}

.overview a {
  color: #1a1a1a;
  text-decoration: underline;
  text-decoration-color: #999999;
  text-underline-offset: 0.2rem;
}

.overview a:hover {
  text-decoration-color: #000000;
}

.overview hr {
  border: none;
  border-top: 1px solid #eeeeee;
  margin: 2rem 0;
}

.highlight {
  background-color: #f2f2f2;
  padding: 0.1rem 0.3rem;
  font-weight: 500;
}

.languages {
  display: flex;
  flex-wrap: wrap;
  gap: 0.5rem;
  margin: 1rem 0 0.5rem 0;
}

.language-tag {
  background-color: #f0f0f0;
  color: #222222;
  padding: 0.3rem 0.8rem;
  border-radius: 0;
  font-family: 'SF Mono', 'Menlo', 'Monaco', 'Cascadia Code', 'Consolas', monospace;
  font-size: 0.9rem;
  border: 1px solid #d0d0d0;
}

.language-note {
  font-style: italic;
  color: #555555;
  margin-top: 0.5rem;
  padding-left: 1rem;
  border-left: 2px solid #cccccc;
}

.category-list, .uses-list {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
  gap: 1rem;
  margin: 1.5rem 0;
}

.category-item, .use-card {
  background-color: #fafafa;
  padding: 1rem;
  border: 1px solid #e0e0e0;
  border-radius: 0;
  box-shadow: none;
}

.category-item strong, .use-card strong {
  display: block;
  margin-bottom: 0.4rem;
  font-weight: 600;
  color: #111111;
}

.philosophy-list {
  list-style-type: none;
  padding-left: 0;
}

.philosophy-list li {
  background-color: #fafafa;
  padding: 0.8rem 1rem;
  margin-bottom: 0.5rem;
  border-left: 3px solid #888888;
  border-radius: 0;
}

.philosophy-list li strong {
  color: #000000;
}

.code {
  font-family: 'SF Mono', 'Menlo', 'Monaco', 'Cascadia Code', 'Consolas', monospace;
  background-color: #f2f2f2;
  color: #1a1a1a;
  padding: 0.2rem 0.4rem;
  border-radius: 0;
  font-size: 0.9em;
  border: 1px solid #e0e0e0;
}

.why-list {
  list-style-type: none;
  padding-left: 0;
}

.why-list li {
  padding: 0.5rem 0;
  padding-left: 1.5rem;
  position: relative;
  border-bottom: 1px solid #f0f0f0;
}

.why-list li::before {
  content: "•";
  position: absolute;
  left: 0;
  color: #888888;
  font-weight: bold;
}
</style>

<div class="overview">

<h1>A Balanced Programming Language Benchmark Suite</h1>

<h2>What is it?</h2>

<p>A collection of <span class="highlight">50 benchmarks</span> for apples-to-apples comparisons across <span class="highlight">22 languages</span>. It focuses on production-like tasks - JSON, Base64, CSV, neural networks, compression, maze A*, graph algorithms, sorting, hashing, interpreters, parallel matmul, and more - rather than microbenchmarks or synthetic loops. Where possible, the same core algorithm is implemented across all languages using idiomatic constructs; for library-based tasks (JSON, Base64, CSV), I use the best practical option for each language. Checksums verify correctness and prevent dead-code elimination. The suite runs monthly with full history, so you can watch compiler performance evolve over time. The goal is not to crown a micro-optimization champion, but to see how well each language's compiler or runtime optimizes clean, readable code.</p>

<h2>Why?</h2>

<p>Most existing benchmarks suffer from the same problems:</p>

<ul>
  <li>They measure micro-optimizations or synthetic loops.</li>
  <li>They mix multiple concepts - making it unclear what's actually being measured.</li>
  <li>They degenerate into assembly or parallelism competitions, rather than clean code.</li>
  <li>The same algorithm is rarely preserved across implementations.</li>
  <li>They measure code that nobody runs in production - where you use standard language constructs, not hand-tuned assembly.</li>
  <li>They rely on unsafe flags (<code>--no-bounds-check</code>) that no one enables in real projects.</li>
  <li>There's no history - most suites are one-off runs.</li>
</ul>

<p><strong>LangArena was built to fix all of this.</strong></p>

<h2>Contenders:</h2>

<div class="languages">
  <span class="language-tag">C</span>
  <span class="language-tag">C++</span>
  <span class="language-tag">Crystal</span>
  <span class="language-tag">Rust</span>
  <span class="language-tag">Go</span>
  <span class="language-tag">Swift</span>
  <span class="language-tag">C#</span>
  <span class="language-tag">Java</span>
  <span class="language-tag">Kotlin</span>
  <span class="language-tag">TypeScript</span>
  <span class="language-tag">Zig</span>
  <span class="language-tag">D</span>
  <span class="language-tag">V</span>
  <span class="language-tag">Julia</span>
  <span class="language-tag">Nim</span>
  <span class="language-tag">F#</span>
  <span class="language-tag">Dart</span>
  <span class="language-tag">Python</span>
  <span class="language-tag">Odin</span>
  <span class="language-tag">Scala</span>
  <span class="language-tag">C3</span>
</div>

<h2>Origin & Approach</h2>

<p>The suite began with my original implementation in Crystal. AI tools assisted in translating it to other languages, but this was far from a one-shot process. Many implementations went through multiple iterations - fixing incorrect checksums, correcting algorithms, rewriting code I didn't like, or investigating unexpected slowness. Others were fine from the start. Either way, every implementation was reviewed for semantic correctness and idiomatic accuracy to ensure fair benchmarking.</p>

<p><strong>Handling Library Differences:</strong> One common objection to any multi-language benchmark is: <em>"You picked the wrong library for language X!"</em> To make this less of an issue, I introduced a <strong>Runtime Score</strong> - a normalized 0–100 metric based on four reference points: <strong>100</strong> (best result), <strong>90</strong> (average of the fastest languages), <strong>50</strong> (overall average), and <strong>0</strong> (worst result). This normalization is applied per benchmark, so each test contributes equally to the final score - regardless of its absolute runtime. The per-benchmark scores are then averaged across all 50 tests to produce the final Runtime Score for each language. This matters because it smooths out outliers. A language that's consistently fast but has one weak library (say, a bad JSON parser) won't be dragged down by a single test. And a language that's slow overall won't get a free pass just because it excelled in a couple of isolated cases. The result is a stable, holistic view of each language's real-world performance - not a ranking that hinges on a single library choice or a lucky benchmark.</p>

<p><strong>Sources:</strong> Benchmark ideas were taken from:</p>
<ul>
  <li><strong>The Computer Language Benchmarks Game</strong></li>
  <li><strong>My own collections:</strong> <a href="https://github.com/kostya/benchmarks">benchmarks</a>, <a href="https://github.com/kostya/jit-benchmarks">jit-benchmarks</a>, <a href="https://github.com/kostya/crystal-benchmarks-game">crystal-benchmarks-game</a>, <a href="https://github.com/kostya/crystal-metric">crystal-metric</a></li>
  <li><strong>Crystal code samples</strong></li>
</ul>

<h2>Core Philosophy</h2>

<ul class="philosophy-list">
  <li><strong>Clean Code:</strong> Benchmarks are written in a clear, idiomatic style that prioritizes readability and maintainability.</li>
  <li><strong>Algorithmic Consistency:</strong> The same core algorithm is used across all languages for each task.</li>
  <li><strong>Standard vs Unsafe Modes:</strong> All benchmarks use standard production compiler flags (safe mode by default). A separate <strong>"Hacking" section</strong> explores unsafe optimizations - disabling bounds checks, runtime checks, and other trade-offs that prioritize speed over safety.</li>
  <li><strong>Testing Language "Muscle":</strong> We measure the <strong>cost of abstractions</strong>. Can a language optimize clean, idiomatic code to efficient machine code? Languages that can (like Rust, Java) prove their compilers are powerful. Those that can't reveal the honest price of their abstractions. Benchmarks like matrix multiplication use <strong>naive implementations</strong> intentionally - not to measure C library calls (e.g., BLAS via numpy), but to see how efficiently the language handles fundamental computational patterns. Because one day, you'll have to write that loop yourself.</li>
  <li><strong>Pull Requests Welcome:</strong> Improvements that maintain this philosophy and fix suboptimal implementations are encouraged.</li>
</ul>

<h2>Benchmarking Methodology</h2>

<p>Each benchmark runs in isolation, with data preparation excluded from timing. JIT-based languages (C#, Java, Julia, etc.) receive a separate warmup phase to reach steady-state performance before measurements begin. This ensures fair comparisons. Checksums verify algorithmic correctness across all implementations.</p>

<h2>Beyond Just Ranking</h2>

<p>This suite is also a practical tool for:</p>

<ul>
  <li><strong>Compiler Tracking</strong> - monitor performance regressions and improvements across compiler versions.</li>
  <li><strong>New Language Evaluation</strong> - get a standardized score to position a new language against established ones.</li>
</ul>

<h2>Hardware</h2>

<p>AMD Ryzen 7 3800X 8-Core, 78GB RAM (x86_64-linux-gnu)</p>

</div>
    `);
}