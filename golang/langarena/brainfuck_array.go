package LangArena

type Tape struct {
	tape []byte
	pos  int
}

func NewTape() Tape {
	return Tape{tape: make([]byte, 30000), pos: 0}
}

func (t *Tape) Get() byte { return t.tape[t.pos] }

func (t *Tape) Inc() {
	t.tape[t.pos] = t.tape[t.pos] + 1
}

func (t *Tape) Dec() {
	t.tape[t.pos] = t.tape[t.pos] - 1
}

func (t *Tape) Advance() {
	t.pos++
	if t.pos >= len(t.tape) {
		t.tape = append(t.tape, 0)
	}
}

func (t *Tape) Devance() {
	if t.pos > 0 {
		t.pos--
	}
}

type Program struct {
	commands []byte
	jumps    []int
}

func NewProgram(text string) *Program {

	commands := make([]byte, 0, len(text))
	for i := 0; i < len(text); i++ {
		c := text[i]

		switch c {
		case '[', ']', '<', '>', '+', '-', ',', '.':
			commands = append(commands, c)
		}
	}

	jumps := make([]int, len(commands))
	stack := make([]int, 0, len(commands)/2)

	for i, cmd := range commands {
		switch cmd {
		case '[':
			stack = append(stack, i)
		case ']':
			if len(stack) > 0 {
				start := stack[len(stack)-1]
				stack = stack[:len(stack)-1]
				jumps[start] = i
				jumps[i] = start
			}
		}
	}

	return &Program{commands: commands, jumps: jumps}
}

func (p *Program) Run() int64 {
	result := int64(0)
	tape := NewTape()
	pc := 0
	cmds := p.commands
	jumps := p.jumps

	for pc < len(cmds) {
		switch cmds[pc] {
		case '+':
			tape.Inc()
		case '-':
			tape.Dec()
		case '>':
			tape.Advance()
		case '<':
			tape.Devance()
		case '[':
			if tape.Get() == 0 {
				pc = jumps[pc]
				continue
			}
		case ']':
			if tape.Get() != 0 {
				pc = jumps[pc]
				continue
			}
		case '.':
			result = (result << 2) + int64(tape.Get())
		}
		pc++
	}
	return result
}

type BrainfuckArray struct {
	BaseBenchmark
	programText string
	warmupText  string
	result      uint32
}

func (b *BrainfuckArray) Name() string {
	return "Brainfuck::Array"
}

func (b *BrainfuckArray) Prepare() {
	b.programText = b.ConfigStr("program")
	b.warmupText = b.ConfigStr("warmup_program")
	b.result = 0
}

func (b *BrainfuckArray) _Run(text string) int64 {
	return NewProgram(text).Run()
}

func (b *BrainfuckArray) Warmup(bench Benchmark) {
	wi := b.WarmupIterations()
	for i := 0; i < wi; i++ {
		b._Run(b.warmupText)
	}
}

func (b *BrainfuckArray) Run(iteration_id int) {
	runResult := b._Run(b.programText)
	b.result += uint32(runResult)
}

func (b *BrainfuckArray) Checksum() uint32 {
	return b.result
}