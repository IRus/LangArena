package main

type Op interface{}
type IncOp struct{}
type DecOp struct{}
type NextOp struct{}
type PrevOp struct{}
type PrintOp struct{}
type LoopOp struct{ ops []Op }

type Program2 struct {
	ops    []Op
	result int64
}

func NewProgram2(code string) *Program2 {
	runes := []rune(code)
	i := 0
	ops := parseProgram(&i, runes)
	return &Program2{ops: ops}
}

func parseProgram(pos *int, runes []rune) []Op {
	res := make([]Op, 0)

	for *pos < len(runes) {
		c := runes[*pos]
		*pos++

		switch c {
		case '+':
			res = append(res, IncOp{})
		case '-':
			res = append(res, DecOp{})
		case '>':
			res = append(res, NextOp{})
		case '<':
			res = append(res, PrevOp{})
		case '.':
			res = append(res, PrintOp{})
		case '[':
			loopOps := parseProgram(pos, runes)
			res = append(res, LoopOp{ops: loopOps})
		case ']':
			return res
		}
	}
	return res
}

func (p *Program2) Run() {
	tape := Tape2{tape: make([]byte, 30000)}
	p.result = 0
	p.runOps(p.ops, &tape)
}

func (p *Program2) runOps(ops []Op, tape *Tape2) {
	for _, op := range ops {
		switch o := op.(type) {
		case IncOp:
			tape.Inc()
		case DecOp:
			tape.Dec()
		case NextOp:
			tape.Next()
		case PrevOp:
			tape.Prev()
		case PrintOp:
			p.result = (p.result << 2) + int64(tape.Get())
		case LoopOp:
			for tape.Get() != 0 {
				p.runOps(o.ops, tape)
			}
		}
	}
}

type Tape2 struct {
	tape []byte
	pos  int
}

func NewTape2() *Tape2 {
	return &Tape2{
		tape: make([]byte, 30000),
		pos:  0,
	}
}

func (t *Tape2) Get() byte {
	return t.tape[t.pos]
}

func (t *Tape2) Inc() {
	t.tape[t.pos]++
}

func (t *Tape2) Dec() {
	t.tape[t.pos]--
}

func (t *Tape2) Next() {
	t.pos++
	if t.pos >= len(t.tape) {
		t.tape = append(t.tape, 0)
	}
}

func (t *Tape2) Prev() {
	if t.pos > 0 {
		t.pos--
	}
}

type BrainfuckRecursion struct {
	BaseBenchmark
	text   string
	result uint32
}

func (b *BrainfuckRecursion) Name() string {
	return "Brainfuck::Recursion"
}

func (b *BrainfuckRecursion) Prepare() {
	b.text = b.ConfigStr("program")
	b.result = 0
}

func (b *BrainfuckRecursion) _Run(text string) int64 {
	prog := NewProgram2(text)
	prog.Run()
	return prog.result
}

func (b *BrainfuckRecursion) Warmup(bench Benchmark) {
	warmupProgram := b.ConfigStr("warmup_program")
	wi := b.WarmupIterations()
	for i := 0; i < wi; i++ {
		b._Run(warmupProgram)
	}
}

func (b *BrainfuckRecursion) Run(iteration_id int) {
	result := b._Run(b.text)
	b.result = (b.result + uint32(result)) & 0xFFFFFFFF
}

func (b *BrainfuckRecursion) Checksum() uint32 {
	return b.result
}
