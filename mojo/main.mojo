from std.time import perf_counter_ns
from std.sys import argv
from std.python import Python, PythonObject
from std.memory import Pointer
from std.memory.alloc import unsafe_alloc
from std.utils.variant import Variant
from max.algorithm.backend.cpu import parallelize
from std.base64 import b64encode, b64decode
from std.math import sqrt, exp


struct Helper:
    comptime IM: Int = 139968
    comptime IA: Int = 3877
    comptime IC: Int = 29573
    comptime INIT: Int = 42

    var last: Int

    def __init__(out self):
        self.last = Self.INIT

    def reset(mut self):
        self.last = Self.INIT

    def next_int(mut self, max: Int) -> Int:
        self.last = (self.last * Self.IA + Self.IC) % Self.IM
        return Int(Float64(self.last) / Float64(Self.IM) * Float64(max))

    def next_float(mut self, max: Float64 = 1.0) -> Float64:
        self.last = (self.last * Self.IA + Self.IC) % Self.IM
        return max * Float64(self.last) / Float64(Self.IM)

    @staticmethod
    def checksum_string(s: String) -> UInt32:
        var hash: UInt32 = 5381
        for b in s.as_bytes():
            hash = ((hash << 5) + hash) + UInt32(b)
        return hash

    @staticmethod
    def checksum_f64(v: Float64) -> UInt32:
        var result = ""
        var val = v
        if val < 0:
            result += "-"
            val = -val

        var int_part = Int(val)
        result += String(int_part)
        result += "."

        var frac_scaled = Int((val - Float64(int_part)) * 10000000.0 + 0.5)
        var fs = String(frac_scaled)
        while fs.byte_length() < 7:
            fs = "0" + fs

        result += fs
        return Helper.checksum_string(result)


struct ConfigEntry(Copyable, Movable):
    var name: String
    var fields: Dict[String, PythonObject]

    def __init__(out self, name: String):
        self.name = name
        self.fields = Dict[String, PythonObject]()


struct Config(Copyable, Movable):
    var entries: List[ConfigEntry]
    var order: List[String]

    def __init__(out self, path: String) raises:
        self.entries = List[ConfigEntry]()
        self.order = List[String]()

        var json_mod = Python.import_module("json")
        var raw: String
        with open(path, "r") as f:
            raw = f.read()
        var data = json_mod.loads(raw)

        var length = Int(py=data.__len__())

        for i in range(length):
            var item = data[i]
            var name = String(py=item["name"])
            var entry = ConfigEntry(name)

            var keys = item.keys()
            for py_key in keys:
                var key = String(py=py_key)
                if key != "name":
                    entry.fields[key] = item[py_key]

            self.order.append(entry.name)
            self.entries.append(entry^)

    def get_i64(self, class_name: String, field_name: String) raises -> Int:
        for i in range(len(self.entries)):
            ref entry = self.entries[i]
            if entry.name == class_name:
                var val = entry.fields.get(field_name)
                if val:
                    return Int(py=val[])
                else:
                    raise Error(
                        String(
                            "field not found: ", field_name, " in ", class_name
                        )
                    )
        raise Error(String("class not found: ", class_name))

    def get_s(self, class_name: String, field_name: String) raises -> String:
        for i in range(len(self.entries)):
            ref entry = self.entries[i]
            if entry.name == class_name:
                var val = entry.fields.get(field_name)
                if val:
                    return String(py=val[])
                else:
                    raise Error(
                        String(
                            "field not found: ", field_name, " in ", class_name
                        )
                    )
        raise Error(String("class not found: ", class_name))


trait Benchmark:
    def prepare(mut self, mut helper: Helper) raises:
        pass

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        ...

    def checksum(mut self) -> UInt32:
        ...

    def warmup_iterations(mut self, config: Config) raises -> Int:
        try:
            return config.get_i64(self.class_name(), "warmup_iterations")
        except:
            var iters = self.iterations(config)
            var w = Int(Float64(iters) * 0.2)
            return 1 if w < 1 else w

    def warmup(mut self, warmup_iters: Int, mut helper: Helper) raises:
        for i in range(warmup_iters):
            self.run(i, helper)

    def iterations(self, config: Config) raises -> Int:
        return config.get_i64(self.class_name(), "iterations")

    def expected_checksum(self, config: Config) raises -> UInt32:
        return UInt32(config.get_i64(self.class_name(), "checksum"))

    def class_name(self) -> String:
        ...


struct _Tape:
    var tape: List[UInt8]
    var pos: Int

    def __init__(out self):
        self.tape = List[UInt8](length=30000, fill=0)
        self.pos = 0

    def get(self) -> UInt8:
        return self.tape[self.pos]

    def inc(mut self):
        self.tape[self.pos] = (self.tape[self.pos] + 1) & 0xFF

    def dec(mut self):
        self.tape[self.pos] = (self.tape[self.pos] - 1) & 0xFF

    def advance(mut self):
        self.pos += 1
        if self.pos >= len(self.tape):
            self.tape.append(0)

    def devance(mut self):
        if self.pos > 0:
            self.pos -= 1


struct BrainfuckArray(Benchmark, Movable):
    var program_text: String
    var warmup_text: String
    var result_val: UInt32

    def __init__(out self, config: Config) raises:
        self.program_text = config.get_s("Brainfuck::Array", "program")
        self.warmup_text = config.get_s("Brainfuck::Array", "warmup_program")
        self.result_val = 0

    def class_name(self) -> String:
        return "Brainfuck::Array"

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var result = self._run_program(self.program_text)
        self.result_val = self.result_val + result

    def warmup(mut self, warmup_iters: Int, mut helper: Helper) raises:
        for _ in range(warmup_iters):
            _ = self._run_program(self.warmup_text)

    def checksum(self) -> UInt32:
        return self.result_val

    def _run_program(self, source: String) -> UInt32:
        var commands = Self._parse_commands(source)
        var jumps = Self._build_jump_array(commands)
        return Self._execute(commands^, jumps^)

    @staticmethod
    def _parse_commands(source: String) -> List[UInt8]:
        var result = List[UInt8]()

        for cp in source.codepoint_slices():
            var s = String(cp)
            if s.byte_length() == 1:
                var b = UInt8(s.as_bytes()[0])

                if (
                    b == 43
                    or b == 45
                    or b == 60
                    or b == 62
                    or b == 91
                    or b == 93
                    or b == 46
                    or b == 44
                ):
                    result.append(b)

        return result^

    @staticmethod
    def _build_jump_array(commands: List[UInt8]) -> List[Int]:
        var jumps = List[Int](length=len(commands), fill=0)
        var stack = List[Int]()

        for i in range(len(commands)):
            var cmd = commands[i]
            if cmd == 91:
                stack.append(i)
            elif cmd == 93:
                if len(stack) == 0:
                    return List[Int]()
                var start = stack[len(stack) - 1]
                _ = stack.pop()
                jumps[start] = i
                jumps[i] = start

        if len(stack) != 0:
            return List[Int]()
        return jumps^

    @staticmethod
    def _execute(commands: List[UInt8], jumps: List[Int]) -> UInt32:
        var tape = _Tape()
        var pc: Int = 0
        var result: UInt32 = 0

        while pc < len(commands):
            var cmd = commands[pc]

            if cmd == 43:
                tape.inc()
            elif cmd == 45:
                tape.dec()
            elif cmd == 62:
                tape.advance()
            elif cmd == 60:
                tape.devance()
            elif cmd == 91:
                if tape.get() == 0:
                    pc = jumps[pc]
            elif cmd == 93:
                if tape.get() != 0:
                    pc = jumps[pc]
            elif cmd == 46:
                result = (result << 2) + UInt32(tape.get())

            pc += 1

        return result


comptime BF_INC = 0
comptime BF_DEC = 1
comptime BF_PREV = 2
comptime BF_NEXT = 3
comptime BF_PRINT = 4
comptime BF_LOOP = 5


struct _BFOp(Copyable, Movable):
    var kind: Int
    var loop_ops: List[_BFOp]

    def __init__(out self, kind: Int):
        self.kind = kind
        self.loop_ops = List[_BFOp]()

    def __init__(out self, kind: Int, var loop_ops: List[_BFOp]):
        self.kind = kind
        self.loop_ops = loop_ops^

    def __deinit__(deinit self):
        pass


struct _BFTape:
    var pos: Int
    var tape: List[UInt8]

    def __init__(out self):
        self.pos = 0
        self.tape = List[UInt8]()
        self.tape.append(0)

    def get(self) -> UInt8:
        return self.tape[self.pos]

    def inc(mut self):
        self.tape[self.pos] = (self.tape[self.pos] + 1) & 0xFF

    def dec(mut self):
        self.tape[self.pos] = (self.tape[self.pos] - 1) & 0xFF

    def prev(mut self):
        if self.pos > 0:
            self.pos -= 1

    def next(mut self):
        self.pos += 1
        if self.pos >= len(self.tape):
            self.tape.append(0)


struct _BFProgram:
    var ops: List[_BFOp]

    def __init__(out self, code: String):
        self.ops = Self._parse(code)

    @staticmethod
    def _parse(code: String) -> List[_BFOp]:
        var chars = List[UInt8]()
        for cp in code.codepoint_slices():
            var s = String(cp)
            if s.byte_length() == 1:
                chars.append(UInt8(s.as_bytes()[0]))

        var pos = 0
        return _BFProgram._parse_ops(chars, pos)

    @staticmethod
    def _parse_ops(chars: List[UInt8], mut pos: Int) -> List[_BFOp]:
        var buf = List[_BFOp]()
        while pos < len(chars):
            var byte = chars[pos]
            pos += 1

            if byte == 45:
                buf.append(_BFOp(BF_DEC))
            elif byte == 43:
                buf.append(_BFOp(BF_INC))
            elif byte == 60:
                buf.append(_BFOp(BF_PREV))
            elif byte == 62:
                buf.append(_BFOp(BF_NEXT))
            elif byte == 46:
                buf.append(_BFOp(BF_PRINT))
            elif byte == 91:
                var inner = _BFProgram._parse_ops(chars, pos)
                buf.append(_BFOp(BF_LOOP, inner^))
            elif byte == 93:
                break
        return buf^

    def run(self) -> Int:
        var tape = _BFTape()
        var result: Int = 0
        Self._execute(self.ops, tape, result)
        return result

    @staticmethod
    def _execute(ops: List[_BFOp], mut tape: _BFTape, mut result: Int):
        for op in ops:
            if op.kind == BF_DEC:
                tape.dec()
            elif op.kind == BF_INC:
                tape.inc()
            elif op.kind == BF_PREV:
                tape.prev()
            elif op.kind == BF_NEXT:
                tape.next()
            elif op.kind == BF_PRINT:
                result = (result << 2) + Int(tape.get())
            elif op.kind == BF_LOOP:
                while tape.get() != 0:
                    _BFProgram._execute(op.loop_ops, tape, result)


struct BrainfuckRecursion(Benchmark, Movable):
    var program_text: String
    var warmup_text: String
    var result_val: UInt32

    def __init__(out self, config: Config) raises:
        self.program_text = config.get_s("Brainfuck::Recursion", "program")
        self.warmup_text = config.get_s(
            "Brainfuck::Recursion", "warmup_program"
        )
        self.result_val = 0

    def class_name(self) -> String:
        return "Brainfuck::Recursion"

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var result = self._run(self.program_text)
        self.result_val = self.result_val + UInt32(result)

    def warmup(mut self, warmup_iters: Int, mut helper: Helper) raises:
        for _ in range(warmup_iters):
            _ = self._run(self.warmup_text)

    def checksum(self) -> UInt32:
        return self.result_val

    def _run(self, text: String) -> Int:
        return _BFProgram(text).run()


struct TreeNodeObj(Copyable, Movable):
    comptime _NodePointer = Optional[Pointer[TreeNodeObj, MutUntrackedOrigin]]

    var item: Int
    var left: Self._NodePointer
    var right: Self._NodePointer

    def __init__(out self, item: Int):
        self.item = item
        self.left = Self._NodePointer()
        self.right = Self._NodePointer()

    def __init__(
        out self,
        item: Int,
        left: Self._NodePointer,
        right: Self._NodePointer,
    ):
        self.item = item
        self.left = left
        self.right = right

    def __deinit__(deinit self):
        if self.left:
            var nn = self.left.value()
            nn.unsafe_deinit_pointee()
            nn.unsafe_free()
        if self.right:
            var nn = self.right.value()
            nn.unsafe_deinit_pointee()
            nn.unsafe_free()

    def sum(ref self) -> UInt32:
        var total = UInt32(self.item + 1)
        if self.left:
            total += self.left.value()[].sum()
        if self.right:
            total += self.right.value()[].sum()
        return total


struct BinarytreesObj(Benchmark, Movable):
    var n: Int
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.n = config.get_i64("Binarytrees::Obj", "depth")
        self.result = 0

    def class_name(self) -> String:
        return "Binarytrees::Obj"

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var root = Self.build_tree(0, self.n)
        self.result += root.sum()

    def checksum(mut self) -> UInt32:
        return self.result

    @staticmethod
    def build_tree(item: Int, depth: Int) raises -> TreeNodeObj:
        if depth > 0:
            var left_ptr = unsafe_alloc[TreeNodeObj](1)
            left_ptr.unsafe_write(
                Self.build_tree(item - (1 << (depth - 1)), depth - 1)
            )
            var right_ptr = unsafe_alloc[TreeNodeObj](1)
            right_ptr.unsafe_write(
                Self.build_tree(item + (1 << (depth - 1)), depth - 1)
            )
            return TreeNodeObj(item, left_ptr, right_ptr)
        return TreeNodeObj(item)


struct TreeNodeArena(Copyable, ImplicitlyCopyable, Movable):
    var item: Int
    var left: Int
    var right: Int

    def __init__(out self, item: Int, left: Int = -1, right: Int = -1):
        self.item = item
        self.left = left
        self.right = right


struct BinarytreesArena(Benchmark, Movable):
    var n: Int
    var result: UInt32
    var arena: List[TreeNodeArena]

    def __init__(out self, config: Config) raises:
        self.n = config.get_i64("Binarytrees::Arena", "depth")
        self.result = 0
        self.arena = List[TreeNodeArena]()

    def class_name(self) -> String:
        return "Binarytrees::Arena"

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.arena = List[TreeNodeArena]()
        _ = self.build_tree(0, self.n)
        self.result += self.sum(0)

    def build_tree(mut self, item: Int, depth: Int) raises -> Int:
        var idx = len(self.arena)
        self.arena.append(TreeNodeArena(item, -1, -1))

        if depth > 0:
            var left_idx = self.build_tree(item - (1 << (depth - 1)), depth - 1)
            var right_idx = self.build_tree(
                item + (1 << (depth - 1)), depth - 1
            )
            ref node = self.arena[idx]
            node.left = left_idx
            node.right = right_idx

        return idx

    def sum(ref self, idx: Int) -> UInt32:
        var node = self.arena[idx]
        var total = UInt32(node.item + 1)
        if node.left >= 0:
            total += self.sum(node.left)
        if node.right >= 0:
            total += self.sum(node.right)
        return total

    def checksum(mut self) -> UInt32:
        return self.result


struct MatmulSingle(Benchmark, Movable):
    var n: Int
    var a: List[List[Float64]]
    var b: List[List[Float64]]
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.n = config.get_i64("Matmul::Single", "n")
        if self.n == 0:
            self.n = 100
        self.a = List[List[Float64]]()
        self.b = List[List[Float64]]()
        self.result = 0

    def class_name(self) -> String:
        return "Matmul::Single"

    def prepare(mut self, mut helper: Helper) raises:
        self.a = Self._matgen(self.n)
        self.b = Self._matgen(self.n)
        self.result = 0

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var c = Self._matmul_sequential(self.a, self.b, self.n)
        var center_value = c[self.n >> 1][self.n >> 1]

        self.result += Helper.checksum_f64(center_value)

    def checksum(mut self) -> UInt32:
        return self.result

    @staticmethod
    def _matgen(n: Int) -> List[List[Float64]]:
        var tmp = 1.0 / Float64(n) / Float64(n)
        var a = List[List[Float64]]()

        for i in range(n):
            var row = List[Float64]()
            for j in range(n):
                row.append(tmp * Float64(i - j) * Float64(i + j))
            a.append(row^)

        return a^

    @staticmethod
    def _transpose(b: List[List[Float64]], n: Int) -> List[List[Float64]]:
        var b_t = List[List[Float64]]()
        for j in range(n):
            var row = List[Float64]()
            for i in range(n):
                row.append(b[i][j])
            b_t.append(row^)
        return b_t^

    @staticmethod
    def _matmul_sequential(
        a: List[List[Float64]], b: List[List[Float64]], n: Int
    ) -> List[List[Float64]]:
        var b_t = MatmulSingle._transpose(b, n)
        var c = List[List[Float64]]()

        for i in range(n):
            var row = List[Float64]()
            ref ai = a[i]

            for j in range(n):
                var s: Float64 = 0.0
                ref b_tj = b_t[j]

                for k in range(n):
                    s += ai[k] * b_tj[k]

                row.append(s)

            c.append(row^)

        return c^


struct MatmulParallel(Benchmark, Movable):
    var n: Int
    var a: List[List[Float64]]
    var b: List[List[Float64]]
    var result: UInt32
    var num_threads: Int
    var config_name: String

    def __init__(
        out self, config: Config, config_name: String, num_threads: Int
    ) raises:
        self.config_name = config_name
        self.num_threads = num_threads
        self.n = config.get_i64(config_name, "n")
        if self.n == 0:
            self.n = 100
        self.a = List[List[Float64]]()
        self.b = List[List[Float64]]()
        self.result = 0

    def class_name(self) -> String:
        return self.config_name

    def prepare(mut self, mut helper: Helper) raises:
        self.a = MatmulSingle._matgen(self.n)
        self.b = MatmulSingle._matgen(self.n)
        self.result = 0

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var c = Self._matmul_parallel(self.a, self.b, self.n, self.num_threads)
        var center_value = c[self.n >> 1][self.n >> 1]

        self.result += Helper.checksum_f64(center_value)

    def checksum(mut self) -> UInt32:
        return self.result

    @staticmethod
    def _matmul_parallel(
        a: List[List[Float64]], b: List[List[Float64]], n: Int, num_threads: Int
    ) -> List[List[Float64]]:
        var b_t = MatmulSingle._transpose(b, n)
        var c = List[List[Float64]]()

        for _ in range(n):
            var row = List[Float64](length=n, fill=0.0)
            c.append(row^)

        var rows_per_worker = (n + num_threads - 1) // num_threads

        def compute_row(
            worker_id: Int,
        ) {imm a, imm b_t, mut c, imm n, imm rows_per_worker}:
            var start_row = worker_id * rows_per_worker
            var end_row = min(start_row + rows_per_worker, n)

            for i in range(start_row, end_row):
                for j in range(n):
                    var s: Float64 = 0.0
                    for k in range(n):
                        s += a[i][k] * b_t[j][k]
                    c[i][j] = s

        parallelize(compute_row, num_threads)
        return c^


struct MatmulT4(Benchmark, Movable):
    var impl: MatmulParallel

    def __init__(out self, config: Config) raises:
        self.impl = MatmulParallel(config, "Matmul::T4", 4)

    def class_name(self) -> String:
        return "Matmul::T4"

    def prepare(mut self, mut helper: Helper) raises:
        self.impl.prepare(helper)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.impl.run(iteration_id, helper)

    def checksum(mut self) -> UInt32:
        return self.impl.checksum()


struct MatmulT8(Benchmark, Movable):
    var impl: MatmulParallel

    def __init__(out self, config: Config) raises:
        self.impl = MatmulParallel(config, "Matmul::T8", 8)

    def class_name(self) -> String:
        return "Matmul::T8"

    def prepare(mut self, mut helper: Helper) raises:
        self.impl.prepare(helper)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.impl.run(iteration_id, helper)

    def checksum(mut self) -> UInt32:
        return self.impl.checksum()


struct MatmulT16(Benchmark, Movable):
    var impl: MatmulParallel

    def __init__(out self, config: Config) raises:
        self.impl = MatmulParallel(config, "Matmul::T16", 16)

    def class_name(self) -> String:
        return "Matmul::T16"

    def prepare(mut self, mut helper: Helper) raises:
        self.impl.prepare(helper)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.impl.run(iteration_id, helper)

    def checksum(mut self) -> UInt32:
        return self.impl.checksum()


struct Base64Encode(Benchmark, Movable):
    var n: Int
    var str: String
    var str2: String
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.n = config.get_i64("Base64::Encode", "size")
        self.str = ""
        self.str2 = ""
        self.result = 0

    def class_name(self) -> String:
        return "Base64::Encode"

    def prepare(mut self, mut helper: Helper) raises:
        self.str = String()
        for _ in range(self.n):
            self.str += "a"
        self.str2 = b64encode(self.str.as_bytes())

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.str2 = b64encode(self.str.as_bytes())
        self.result += UInt32(self.str2.byte_length())

    def checksum(mut self) -> UInt32:
        var prefix1 = ""
        var prefix2 = ""
        if self.str.byte_length() >= 4:
            prefix1 = (
                String(self.str[byte=0])
                + String(self.str[byte=1])
                + String(self.str[byte=2])
                + String(self.str[byte=3])
            )
        if self.str2.byte_length() >= 4:
            prefix2 = (
                String(self.str2[byte=0])
                + String(self.str2[byte=1])
                + String(self.str2[byte=2])
                + String(self.str2[byte=3])
            )
        var desc = String(
            "encode ", prefix1, "... to ", prefix2, "...: ", self.result
        )
        return Helper.checksum_string(desc)


struct Base64Decode(Benchmark, Movable):
    var n: Int
    var str2: String
    var str3: List[UInt8]
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.n = config.get_i64("Base64::Decode", "size")
        self.str2 = ""
        self.str3 = []
        self.result = 0

    def class_name(self) -> String:
        return "Base64::Decode"

    def prepare(mut self, mut helper: Helper) raises:
        var str = String()
        for _ in range(self.n):
            str += "a"
        self.str2 = b64encode(str.as_bytes())
        self.str3 = b64decode(StringSlice(self.str2))

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.str3 = b64decode(StringSlice(self.str2))
        self.result += UInt32(self.str3.byte_length())

    def checksum(mut self) -> UInt32:
        var prefix1 = ""
        var prefix2 = ""
        if self.str2.byte_length() >= 4:
            prefix1 = (
                String(self.str2[byte=0])
                + String(self.str2[byte=1])
                + String(self.str2[byte=2])
                + String(self.str2[byte=3])
            )
        if self.str3.byte_length() >= 4:
            prefix2 = (
                String(chr(Int(self.str3[0])))
                + String(chr(Int(self.str3[1])))
                + String(chr(Int(self.str3[2])))
                + String(chr(Int(self.str3[3])))
            )
        var desc = String(
            "decode ", prefix1, "... to ", prefix2, "...: ", self.result
        )
        return Helper.checksum_string(desc)


struct Fannkuchredux(Benchmark, Movable):
    var n: Int
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.n = config.get_i64("CLBG::Fannkuchredux", "n")
        self.result = 0

    def class_name(self) -> String:
        return "CLBG::Fannkuchredux"

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var a, b = Self.fannkuchredux(self.n)
        self.result += UInt32(a * 100 + b)

    def checksum(mut self) -> UInt32:
        return self.result

    @staticmethod
    def fannkuchredux(n: Int) -> Tuple[Int, Int]:
        var perm1 = List[Int](capacity=32)
        var perm = List[Int](capacity=32)
        var count = List[Int](capacity=32)

        for i in range(32):
            perm1.append(i)
            perm.append(0)
            count.append(0)

        var max_flips = 0
        var checksum = 0
        var r = n
        var perm_count = 0

        while True:
            while r > 1:
                count[r - 1] = r
                r -= 1

            for i in range(n):
                perm[i] = perm1[i]

            var flips = 0
            var k = perm[0]
            while k != 0:
                var k2 = (k + 1) >> 1
                for i in range(k2):
                    var j = k - i
                    var tmp = perm[i]
                    perm[i] = perm[j]
                    perm[j] = tmp
                flips += 1
                k = perm[0]

            if flips > max_flips:
                max_flips = flips

            if perm_count % 2 == 0:
                checksum += flips
            else:
                checksum -= flips

            while True:
                if r == n:
                    return (checksum, max_flips)

                var perm0 = perm1[0]
                for i in range(r):
                    var j = i + 1
                    var tmp = perm1[i]
                    perm1[i] = perm1[j]
                    perm1[j] = tmp

                perm1[r] = perm0
                count[r] -= 1
                if count[r] > 0:
                    break
                r += 1

            perm_count += 1


struct Spectralnorm(Benchmark, Movable):
    var size: Int
    var u: List[Float64]
    var v: List[Float64]
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.size = config.get_i64("CLBG::Spectralnorm", "size")
        self.u = List[Float64](capacity=self.size)
        self.v = List[Float64](capacity=self.size)
        for _ in range(self.size):
            self.u.append(1.0)
            self.v.append(1.0)
        self.result = 0

    def class_name(self) -> String:
        return "CLBG::Spectralnorm"

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.v = Self.eval_AtA_times_u(self.u, self.size)
        self.u = Self.eval_AtA_times_u(self.v, self.size)

    def checksum(mut self) -> UInt32:
        var vBv: Float64 = 0.0
        var vv: Float64 = 0.0
        for i in range(self.size):
            vBv += self.u[i] * self.v[i]
            vv += self.v[i] * self.v[i]
        return Helper.checksum_f64(sqrt(vBv / vv))

    @staticmethod
    def eval_A(i: Int, j: Int) -> Float64:
        return 1.0 / Float64((i + j) * (i + j + 1) / 2 + i + 1)

    @staticmethod
    def eval_A_times_u(u: List[Float64], size: Int) -> List[Float64]:
        var result = List[Float64](capacity=size)
        for i in range(size):
            var v: Float64 = 0.0
            for j in range(size):
                v += Spectralnorm.eval_A(i, j) * u[j]
            result.append(v)
        return result^

    @staticmethod
    def eval_At_times_u(u: List[Float64], size: Int) -> List[Float64]:
        var result = List[Float64](capacity=size)
        for i in range(size):
            var v: Float64 = 0.0
            for j in range(size):
                v += Spectralnorm.eval_A(j, i) * u[j]
            result.append(v)
        return result^

    @staticmethod
    def eval_AtA_times_u(u: List[Float64], size: Int) -> List[Float64]:
        var tmp = Spectralnorm.eval_A_times_u(u, size)
        return Spectralnorm.eval_At_times_u(tmp, size)


struct Mandelbrot(Benchmark, Movable):
    var w: Int
    var h: Int
    var result_data: List[UInt8]
    var result_val: UInt32

    def __init__(out self, config: Config) raises:
        self.w = config.get_i64("CLBG::Mandelbrot", "w")
        self.h = config.get_i64("CLBG::Mandelbrot", "h")
        self.result_data = List[UInt8]()
        self.result_val = 0

    def class_name(self) -> String:
        return "CLBG::Mandelbrot"

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var header = (
            String("P4\n") + String(self.w) + " " + String(self.h) + "\n"
        )
        for b in header.as_bytes():
            self.result_data.append(b)

        comptime ITER: Int = 50
        comptime LIMIT: Float64 = 2.0
        comptime LIMIT_SQ = LIMIT * LIMIT

        var bit_num: Int = 0
        var byte_acc: UInt8 = 0

        for y in range(self.h):
            for x in range(self.w):
                var zr: Float64 = 0.0
                var zi: Float64 = 0.0
                var tr: Float64 = 0.0
                var ti: Float64 = 0.0
                var cr = 2.0 * Float64(x) / Float64(self.w) - 1.5
                var ci = 2.0 * Float64(y) / Float64(self.h) - 1.0

                var i = 0
                while i < ITER and tr + ti <= LIMIT_SQ:
                    zi = 2.0 * zr * zi + ci
                    zr = tr - ti + cr
                    tr = zr * zr
                    ti = zi * zi
                    i += 1

                byte_acc <<= 1
                if tr + ti <= LIMIT_SQ:
                    byte_acc |= 0x01

                bit_num += 1

                if bit_num == 8:
                    self.result_data.append(byte_acc)
                    byte_acc = 0
                    bit_num = 0
                elif x == self.w - 1:
                    byte_acc <<= UInt8(8 - (self.w % 8))
                    self.result_data.append(byte_acc)
                    byte_acc = 0
                    bit_num = 0

    def checksum(mut self) -> UInt32:
        var hash: UInt32 = 5381
        for b in self.result_data:
            hash = ((hash << 5) + hash) + UInt32(b)
        return hash


comptime SOLAR_MASS = 4.0 * 3.141592653589793 * 3.141592653589793
comptime DAYS_PER_YEAR = 365.24


struct _Planet(Copyable, ImplicitlyCopyable, Movable):
    var x: Float64
    var y: Float64
    var z: Float64
    var vx: Float64
    var vy: Float64
    var vz: Float64
    var mass: Float64

    def __init__(
        out self,
        x: Float64,
        y: Float64,
        z: Float64,
        vx: Float64,
        vy: Float64,
        vz: Float64,
        mass: Float64,
    ):
        self.x = x
        self.y = y
        self.z = z
        self.vx = vx * DAYS_PER_YEAR
        self.vy = vy * DAYS_PER_YEAR
        self.vz = vz * DAYS_PER_YEAR
        self.mass = mass * SOLAR_MASS


struct Nbody(Benchmark, Movable):
    var bodies: List[_Planet]
    var v1: Float64
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.bodies = Self._init_bodies()
        self.v1 = 0.0
        self.result = 0

    def class_name(self) -> String:
        return "CLBG::Nbody"

    def prepare(mut self, mut helper: Helper) raises:
        Self._offset_momentum(self.bodies)
        self.v1 = Self._energy(self.bodies)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var nbodies = len(self.bodies)
        var dt = 0.01

        for _ in range(1000):
            for i in range(nbodies):
                Self._move_from_i(self.bodies, i, dt)

    def checksum(self) -> UInt32:
        var v2 = Self._energy(self.bodies)
        return (Helper.checksum_f64(self.v1) << 5) & Helper.checksum_f64(v2)

    @staticmethod
    def _init_bodies() -> List[_Planet]:
        var bodies = List[_Planet]()
        bodies.append(_Planet(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0))
        bodies.append(
            _Planet(
                4.84143144246472090e00,
                -1.16032004402742839e00,
                -1.03622044471123109e-01,
                1.66007664274403694e-03,
                7.69901118419740425e-03,
                -6.90460016972063023e-05,
                9.54791938424326609e-04,
            )
        )
        bodies.append(
            _Planet(
                8.34336671824457987e00,
                4.12479856412430479e00,
                -4.03523417114321381e-01,
                -2.76742510726862411e-03,
                4.99852801234917238e-03,
                2.30417297573763929e-05,
                2.85885980666130812e-04,
            )
        )
        bodies.append(
            _Planet(
                1.28943695621391310e01,
                -1.51111514016986312e01,
                -2.23307578892655734e-01,
                2.96460137564761618e-03,
                2.37847173959480950e-03,
                -2.96589568540237556e-05,
                4.36624404335156298e-05,
            )
        )
        bodies.append(
            _Planet(
                1.53796971148509165e01,
                -2.59193146099879641e01,
                1.79258772950371181e-01,
                2.68067772490389322e-03,
                1.62824170038242295e-03,
                -9.51592254519715870e-05,
                5.15138902046611451e-05,
            )
        )
        return bodies^

    @staticmethod
    def _move_from_i(mut bodies: List[_Planet], idx: Int, dt: Float64):
        var b = bodies[idx]
        var i = idx + 1
        var nbodies = len(bodies)

        while i < nbodies:
            var b2 = bodies[i]
            var dx = b.x - b2.x
            var dy = b.y - b2.y
            var dz = b.z - b2.z

            var distance = sqrt(dx * dx + dy * dy + dz * dz)
            var mag = dt / (distance * distance * distance)
            var b_mass_mag = b.mass * mag
            var b2_mass_mag = b2.mass * mag

            b.vx -= dx * b2_mass_mag
            b.vy -= dy * b2_mass_mag
            b.vz -= dz * b2_mass_mag
            b2.vx += dx * b_mass_mag
            b2.vy += dy * b_mass_mag
            b2.vz += dz * b_mass_mag

            bodies[i] = b2
            i += 1

        b.x += dt * b.vx
        b.y += dt * b.vy
        b.z += dt * b.vz
        bodies[idx] = b

    @staticmethod
    def _energy(bodies: List[_Planet]) -> Float64:
        var e: Float64 = 0.0
        var nbodies = len(bodies)

        for i in range(nbodies):
            var b = bodies[i]
            e += 0.5 * b.mass * (b.vx * b.vx + b.vy * b.vy + b.vz * b.vz)
            for j in range(i + 1, nbodies):
                var b2 = bodies[j]
                var dx = b.x - b2.x
                var dy = b.y - b2.y
                var dz = b.z - b2.z
                var distance = sqrt(dx * dx + dy * dy + dz * dz)
                e -= (b.mass * b2.mass) / distance
        return e

    @staticmethod
    def _offset_momentum(mut bodies: List[_Planet]):
        var px: Float64 = 0.0
        var py: Float64 = 0.0
        var pz: Float64 = 0.0

        for i in range(len(bodies)):
            var b = bodies[i]
            px += b.vx * b.mass
            py += b.vy * b.mass
            pz += b.vz * b.mass

        var sun = bodies[0]
        sun.vx = -px / SOLAR_MASS
        sun.vy = -py / SOLAR_MASS
        sun.vz = -pz / SOLAR_MASS
        bodies[0] = sun


def generate_pair_strings(
    mut helper: Helper, n: Int, m: Int
) -> List[Tuple[String, String]]:
    var pairs = List[Tuple[String, String]]()
    var chars = List[String]()
    chars.append("a")
    chars.append("b")
    chars.append("c")
    chars.append("d")
    chars.append("e")
    chars.append("f")
    chars.append("g")
    chars.append("h")
    chars.append("i")
    chars.append("j")

    for _ in range(n):
        var len1 = helper.next_int(m) + 4
        var len2 = helper.next_int(m) + 4

        var str1 = ""
        for _ in range(len1):
            str1 += chars[helper.next_int(10)]

        var str2 = ""
        for _ in range(len2):
            str2 += chars[helper.next_int(10)]

        pairs.append((str1, str2))

    return pairs^


struct DistanceJaro(Benchmark, Movable):
    var count: Int
    var size: Int
    var pairs: List[Tuple[String, String]]
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.count = config.get_i64("Distance::Jaro", "count")
        self.size = config.get_i64("Distance::Jaro", "size")
        self.pairs = List[Tuple[String, String]]()
        self.result = 0

    def class_name(self) -> String:
        return "Distance::Jaro"

    def prepare(mut self, mut helper: Helper) raises:
        self.pairs = generate_pair_strings(helper, self.count, self.size)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        for i in range(len(self.pairs)):
            var pair = self.pairs[i]
            self.result += UInt32(Self.jaro(pair[0], pair[1]) * 1000)

    def checksum(mut self) -> UInt32:
        return self.result

    @staticmethod
    def jaro(s1: String, s2: String) -> Float64:
        var b1 = s1.as_bytes()
        var b2 = s2.as_bytes()
        var len1 = s1.byte_length()
        var len2 = s2.byte_length()

        if len1 == 0 or len2 == 0:
            return 0.0

        var match_dist = max(len1, len2) // 2 - 1
        if match_dist < 0:
            match_dist = 0

        var s1_matches = List[Bool](length=len1, fill=False)
        var s2_matches = List[Bool](length=len2, fill=False)

        var matches: Int = 0
        for i in range(len1):
            var start = max(0, i - match_dist)
            var fin = min(len2 - 1, i + match_dist)

            for j in range(start, fin + 1):
                if not s2_matches[j] and b1[i] == b2[j]:
                    s1_matches[i] = True
                    s2_matches[j] = True
                    matches += 1
                    break

        if matches == 0:
            return 0.0

        var k: Int = 0
        var transpositions: Int = 0
        for i in range(len1):
            if s1_matches[i]:
                while k < len2 and not s2_matches[k]:
                    k += 1
                if k < len2:
                    if b1[i] != b2[k]:
                        transpositions += 1
                    k += 1

        transpositions //= 2

        var m = Float64(matches)
        return (
            m / Float64(len1)
            + m / Float64(len2)
            + (m - Float64(transpositions)) / m
        ) / 3.0


struct DistanceNGram(Benchmark, Movable):
    var count: Int
    var size: Int
    var pairs: List[Tuple[String, String]]
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.count = config.get_i64("Distance::NGram", "count")
        self.size = config.get_i64("Distance::NGram", "size")
        self.pairs = List[Tuple[String, String]]()
        self.result = 0

    def class_name(self) -> String:
        return "Distance::NGram"

    def prepare(mut self, mut helper: Helper) raises:
        self.pairs = generate_pair_strings(helper, self.count, self.size)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        for i in range(len(self.pairs)):
            var pair = self.pairs[i]
            self.result += UInt32(Self.ngram(pair[0], pair[1]) * 1000)

    def checksum(mut self) -> UInt32:
        return self.result

    @staticmethod
    def ngram(s1: String, s2: String) -> Float64:
        var bytes1 = s1.as_bytes()
        var bytes2 = s2.as_bytes()
        var len1 = s1.byte_length()
        var len2 = s2.byte_length()

        var grams1 = Dict[UInt32, Int]()

        for i in range(len1 - 3):
            var gram = (
                (UInt32(bytes1[i]) << 24)
                | (UInt32(bytes1[i + 1]) << 16)
                | (UInt32(bytes1[i + 2]) << 8)
                | UInt32(bytes1[i + 3])
            )
            var existing = grams1.get(gram).or_else(0)
            grams1[gram] = existing + 1

        var grams2 = Dict[UInt32, Int]()
        var intersection: Int = 0

        for i in range(len2 - 3):
            var gram = (
                (UInt32(bytes2[i]) << 24)
                | (UInt32(bytes2[i + 1]) << 16)
                | (UInt32(bytes2[i + 2]) << 8)
                | UInt32(bytes2[i + 3])
            )
            var existing2 = grams2.get(gram).or_else(0)
            grams2[gram] = existing2 + 1

            var v = grams1.get(gram).or_else(0)
            if v > 0:
                var g2_val = grams2.get(gram).or_else(0)
                if g2_val <= v:
                    intersection += 1

        var total = len(grams1) + len(grams2)
        if total > 0:
            return Float64(intersection) / Float64(total)
        return 0.0


comptime MAZE_WALL = 0
comptime MAZE_SPACE = 1
comptime MAZE_START = 2
comptime MAZE_FINISH = 3
comptime MAZE_BORDER = 4


struct _MazeCell(Copyable, Movable):
    var kind: Int
    var x: Int
    var y: Int
    var neighbors: List[Tuple[Int, Int]]
    var neighbor_count: Int

    def __init__(out self, x: Int, y: Int):
        self.kind = MAZE_WALL
        self.x = x
        self.y = y
        self.neighbors = List[Tuple[Int, Int]](capacity=4)
        self.neighbor_count = 0

    def is_walkable(self) -> Bool:
        return (
            self.kind == MAZE_SPACE
            or self.kind == MAZE_START
            or self.kind == MAZE_FINISH
        )

    def is_wall(self) -> Bool:
        return self.kind == MAZE_WALL

    def is_space(self) -> Bool:
        return self.kind == MAZE_SPACE

    def reset(mut self):
        if self.kind == MAZE_SPACE:
            self.kind = MAZE_WALL


struct _PriorityQueue(Movable):
    var heap: List[Tuple[Int, Int]]
    var best: List[Int]

    def __init__(out self, size: Int):
        self.heap = List[Tuple[Int, Int]]()
        self.best = List[Int](length=size, fill=2147483647)

    def empty(self) -> Bool:
        return len(self.heap) == 0

    def push(mut self, vertex: Int, priority: Int):
        if priority >= self.best[vertex]:
            return
        self.best[vertex] = priority
        self.heap.append((vertex, priority))
        var i = len(self.heap) - 1
        while i > 0:
            var parent = (i - 1) // 2
            if self.heap[parent][1] <= priority:
                break
            var tmp = self.heap[i]
            self.heap[i] = self.heap[parent]
            self.heap[parent] = tmp
            i = parent

    def pop(mut self) -> Tuple[Int, Int]:
        var min_val = self.heap[0]
        var last = self.heap[len(self.heap) - 1]
        _ = self.heap.pop()
        if len(self.heap) > 0:
            self.heap[0] = last
            var i = 0
            while True:
                var left = 2 * i + 1
                var right = 2 * i + 2
                var smallest = i
                if (
                    left < len(self.heap)
                    and self.heap[left][1] < self.heap[smallest][1]
                ):
                    smallest = left
                if (
                    right < len(self.heap)
                    and self.heap[right][1] < self.heap[smallest][1]
                ):
                    smallest = right
                if smallest == i:
                    break
                var tmp = self.heap[i]
                self.heap[i] = self.heap[smallest]
                self.heap[smallest] = tmp
                i = smallest
        return min_val


struct MazeGenerator(Benchmark, Movable):
    var w: Int
    var h: Int
    var cells: List[List[_MazeCell]]
    var start_x: Int
    var start_y: Int
    var finish_x: Int
    var finish_y: Int
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.w = config.get_i64("Maze::Generator", "w")
        self.h = config.get_i64("Maze::Generator", "h")

        if self.w < 5:
            self.w = 5
        if self.h < 5:
            self.h = 5

        self.start_x = 1
        self.start_y = 1
        self.finish_x = self.w - 2
        self.finish_y = self.h - 2
        self.cells = List[List[_MazeCell]]()
        self.result = 0

    def class_name(self) -> String:
        return "Maze::Generator"

    def prepare(mut self, mut helper: Helper) raises:
        self.cells = List[List[_MazeCell]]()
        for y in range(self.h):
            var row = List[_MazeCell](capacity=self.w)
            for x in range(self.w):
                row.append(_MazeCell(x, y))
            self.cells.append(row^)

        self.cells[self.start_y][self.start_x].kind = MAZE_START
        self.cells[self.finish_y][self.finish_x].kind = MAZE_FINISH

        self._link_neighbors(helper)
        self.result = 0

    def _link_neighbors(mut self, mut helper: Helper):
        for y in range(self.h):
            for x in range(self.w):
                if x == 0 or y == 0 or x == self.w - 1 or y == self.h - 1:
                    self.cells[y][x].kind = MAZE_BORDER

        for y in range(1, self.h - 1):
            for x in range(1, self.w - 1):
                ref cell = self.cells[y][x]

                cell.neighbors = List[Tuple[Int, Int]](capacity=4)
                cell.neighbor_count = 0

                cell.neighbors.append((y - 1, x))
                cell.neighbors.append((y + 1, x))
                cell.neighbors.append((y, x + 1))
                cell.neighbors.append((y, x - 1))
                cell.neighbor_count = 4

                for _ in range(4):
                    var i = helper.next_int(4)
                    var j = helper.next_int(4)
                    if i != j:
                        var tmp = cell.neighbors[i]
                        cell.neighbors[i] = cell.neighbors[j]
                        cell.neighbors[j] = tmp

        self.cells[self.start_y][self.start_x].kind = MAZE_START
        self.cells[self.finish_y][self.finish_x].kind = MAZE_FINISH

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self._reset()
        self._generate()
        var mid_y = self.h >> 1
        var mid_x = self.w >> 1
        self.result += UInt32(self.cells[mid_y][mid_x].kind)

    def checksum(self) -> UInt32:
        var hash: UInt32 = 2166136261
        var prime: UInt32 = 16777619
        for y in range(self.h):
            for x in range(self.w):
                if self.cells[y][x].kind == MAZE_SPACE:
                    var j_sq = UInt32(x * y)
                    hash = (hash ^ j_sq) * prime
        return self.result + hash

    def _reset(mut self):
        for y in range(self.h):
            for x in range(self.w):
                self.cells[y][x].reset()
        self.cells[self.start_y][self.start_x].kind = MAZE_START
        self.cells[self.finish_y][self.finish_x].kind = MAZE_FINISH

    def _generate(mut self):
        var start_neighbors = self.cells[self.start_y][
            self.start_x
        ].neighbors.copy()

        for i in range(4):
            var neighbor = start_neighbors[i]
            var ny = neighbor[0]
            var nx = neighbor[1]
            if self.cells[ny][nx].kind == MAZE_WALL:
                self._dig(ny, nx)

        var finish_neighbors = self.cells[self.finish_y][
            self.finish_x
        ].neighbors.copy()

        for i in range(4):
            var neighbor = finish_neighbors[i]
            var ny = neighbor[0]
            var nx = neighbor[1]
            if self.cells[ny][nx].kind == MAZE_WALL:
                self._ensure_open_finish(ny, nx)

    def _dig(mut self, start_y: Int, start_x: Int):
        var stack = List[Tuple[Int, Int]](capacity=self.w * self.h)
        stack.append((start_y, start_x))

        while len(stack) > 0:
            var pos = stack.pop()
            var y = pos[0]
            var x = pos[1]

            var walkable_count = 0

            for i in range(4):
                var neighbor = self.cells[y][x].neighbors[i]
                var ny = neighbor[0]
                var nx = neighbor[1]
                if self.cells[ny][nx].is_walkable():
                    walkable_count += 1

            if walkable_count == 1:
                self.cells[y][x].kind = MAZE_SPACE

                for i in range(4):
                    var neighbor = self.cells[y][x].neighbors[i]
                    var ny = neighbor[0]
                    var nx = neighbor[1]
                    if self.cells[ny][nx].kind == MAZE_WALL:
                        stack.append((ny, nx))

    def _ensure_open_finish(mut self, y: Int, x: Int):
        self.cells[y][x].kind = MAZE_SPACE

        var walkable_count = 0

        for i in range(4):
            var neighbor = self.cells[y][x].neighbors[i]
            var ny = neighbor[0]
            var nx = neighbor[1]
            if self.cells[ny][nx].is_walkable():
                walkable_count += 1

        if walkable_count > 1:
            return

        for i in range(4):
            var neighbor = self.cells[y][x].neighbors[i]
            var ny = neighbor[0]
            var nx = neighbor[1]
            if self.cells[ny][nx].kind == MAZE_WALL:
                self._ensure_open_finish(ny, nx)


struct MazeBFS(Benchmark, Movable):
    var generator: MazeGenerator
    var result: UInt32
    var path: List[Tuple[Int, Int]]

    def __init__(out self, config: Config) raises:
        self.generator = MazeGenerator(config)
        self.result = 0
        self.path = List[Tuple[Int, Int]]()

    def class_name(self) -> String:
        return "Maze::BFS"

    def prepare(mut self, mut helper: Helper) raises:
        self.generator.prepare(helper)
        self.generator._generate()

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.path = Self._bfs(
            self.generator,
            self.generator.start_x,
            self.generator.start_y,
            self.generator.finish_x,
            self.generator.finish_y,
        )
        self.result += UInt32(len(self.path))

    def checksum(self) -> UInt32:
        return self.result + Self._mid_cell_checksum(self.path)

    @staticmethod
    def _mid_cell_checksum(path: List[Tuple[Int, Int]]) -> UInt32:
        if len(path) == 0:
            return 0
        var mid = len(path) // 2
        var cell = path[mid]
        return UInt32(cell[0] * cell[1])

    @staticmethod
    def _bfs(
        maze: MazeGenerator,
        sx: Int,
        sy: Int,
        tx: Int,
        ty: Int,
    ) -> List[Tuple[Int, Int]]:
        if sx == tx and sy == ty:
            var r = List[Tuple[Int, Int]]()
            r.append((sx, sy))
            return r^

        var visited = List[List[Bool]]()
        for _ in range(maze.h):
            var row = List[Bool](length=maze.w, fill=False)
            visited.append(row^)

        var queue = List[Int]()
        var path_nodes = List[Tuple[Int, Int, Int]]()

        visited[sy][sx] = True
        path_nodes.append((sx, sy, -1))
        queue.append(0)

        var q_idx = 0
        while q_idx < len(queue):
            var path_id = queue[q_idx]
            q_idx += 1
            var cur = path_nodes[path_id]
            var cx = cur[0]
            var cy = cur[1]

            ref cell = maze.cells[cy][cx]

            for i in range(cell.neighbor_count):
                var neighbor_coords = cell.neighbors[i]
                var ny = neighbor_coords[0]
                var nx = neighbor_coords[1]

                if nx == tx and ny == ty:
                    var result = List[Tuple[Int, Int]]()
                    result.append((tx, ty))
                    var current = path_id
                    while current >= 0:
                        var p = path_nodes[current]
                        result.append((p[0], p[1]))
                        current = p[2]
                    var reversed_result = List[Tuple[Int, Int]]()
                    for i in range(len(result) - 1, -1, -1):
                        reversed_result.append(result[i])
                    return reversed_result^

                if maze.cells[ny][nx].is_walkable() and not visited[ny][nx]:
                    visited[ny][nx] = True
                    path_nodes.append((nx, ny, path_id))
                    queue.append(len(path_nodes) - 1)

        return List[Tuple[Int, Int]]()


struct MazeAStar(Benchmark, Movable):
    var generator: MazeGenerator
    var result: UInt32
    var path: List[Tuple[Int, Int]]

    def __init__(out self, config: Config) raises:
        self.generator = MazeGenerator(config)
        self.result = 0
        self.path = List[Tuple[Int, Int]]()

    def class_name(self) -> String:
        return "Maze::AStar"

    def prepare(mut self, mut helper: Helper) raises:
        self.generator.prepare(helper)
        self.generator._generate()

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.path = Self._astar(
            self.generator,
            self.generator.start_x,
            self.generator.start_y,
            self.generator.finish_x,
            self.generator.finish_y,
        )
        self.result += UInt32(len(self.path))

    def checksum(self) -> UInt32:
        return self.result + Self._mid_cell_checksum(self.path)

    @staticmethod
    def _mid_cell_checksum(path: List[Tuple[Int, Int]]) -> UInt32:
        if len(path) == 0:
            return 0
        var mid = len(path) // 2
        var cell = path[mid]
        return UInt32(cell[0] * cell[1])

    @staticmethod
    def _heuristic(ax: Int, ay: Int, bx: Int, by: Int) -> Int:
        var dx = ax - bx
        var dy = ay - by
        if dx < 0:
            dx = -dx
        if dy < 0:
            dy = -dy
        return dx + dy

    @staticmethod
    def _astar(
        maze: MazeGenerator,
        sx: Int,
        sy: Int,
        tx: Int,
        ty: Int,
    ) -> List[Tuple[Int, Int]]:
        if sx == tx and sy == ty:
            var r = List[Tuple[Int, Int]]()
            r.append((sx, sy))
            return r^

        var size = maze.w * maze.h
        var start_idx = sy * maze.w + sx
        var target_idx = ty * maze.w + tx

        var came_from = List[Int](length=size, fill=-1)
        var g_score = List[Int](length=size, fill=2147483647)
        var best_f = List[Int](length=size, fill=2147483647)

        var open_set = _PriorityQueue(size)

        g_score[start_idx] = 0
        var f_start = MazeAStar._heuristic(sx, sy, tx, ty)
        open_set.push(start_idx, f_start)
        best_f[start_idx] = f_start

        while not open_set.empty():
            var cur = open_set.pop()
            var current_idx = cur[0]
            var f_val = cur[1]

            if f_val != best_f[current_idx]:
                continue

            if current_idx == target_idx:
                var result = List[Tuple[Int, Int]]()
                var cur_idx = current_idx
                while cur_idx != -1:
                    var cy = cur_idx // maze.w
                    var cx = cur_idx % maze.w
                    result.append((cx, cy))
                    cur_idx = came_from[cur_idx]

                var reversed_result = List[Tuple[Int, Int]]()
                for i in range(len(result) - 1, -1, -1):
                    reversed_result.append(result[i])
                return reversed_result^

            var cy = current_idx // maze.w
            var cx = current_idx % maze.w
            var current_g = g_score[current_idx]

            ref cell = maze.cells[cy][cx]

            for i in range(cell.neighbor_count):
                var neighbor_coords = cell.neighbors[i]
                var ny = neighbor_coords[0]
                var nx = neighbor_coords[1]

                if not maze.cells[ny][nx].is_walkable():
                    continue

                var neighbor_idx = ny * maze.w + nx
                var tentative_g = current_g + 1

                if tentative_g < g_score[neighbor_idx]:
                    came_from[neighbor_idx] = current_idx
                    g_score[neighbor_idx] = tentative_g
                    var f_new = tentative_g + MazeAStar._heuristic(
                        nx, ny, tx, ty
                    )
                    if f_new < best_f[neighbor_idx]:
                        best_f[neighbor_idx] = f_new
                        open_set.push(neighbor_idx, f_new)

        return List[Tuple[Int, Int]]()


struct HashSHA256(Benchmark, Movable):
    var size: Int
    var data: List[UInt8]
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.size = config.get_i64("Hash::SHA256", "size")
        self.data = List[UInt8]()
        self.result = 0

    def class_name(self) -> String:
        return "Hash::SHA256"

    def prepare(mut self, mut helper: Helper) raises:
        self.data = List[UInt8]()
        for _ in range(self.size):
            self.data.append(UInt8(helper.next_int(256)))

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.result += Self._simple_sha256(self.data)

    def checksum(mut self) -> UInt32:
        return self.result

    @staticmethod
    def _simple_sha256(data: List[UInt8]) -> UInt32:
        var hashes = List[UInt32]()
        hashes.append(0x6A09E667)
        hashes.append(0xBB67AE85)
        hashes.append(0x3C6EF372)
        hashes.append(0xA54FF53A)
        hashes.append(0x510E527F)
        hashes.append(0x9B05688C)
        hashes.append(0x1F83D9AB)
        hashes.append(0x5BE0CD19)

        for i in range(len(data)):
            var hash_idx = i & 7
            var hash = hashes[hash_idx]
            hash = ((hash << 5) + hash) + UInt32(data[i])
            hash = (hash + (hash << 10)) ^ (hash >> 6)
            hashes[hash_idx] = hash

        var h0 = hashes[0]
        var b0 = (h0 >> 24) & 0xFF
        var b1 = (h0 >> 16) & 0xFF
        var b2 = (h0 >> 8) & 0xFF
        var b3 = h0 & 0xFF

        return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0


struct HashCRC32(Benchmark, Movable):
    var size: Int
    var data: List[UInt8]
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.size = config.get_i64("Hash::CRC32", "size")
        self.data = List[UInt8]()
        self.result = 0

    def class_name(self) -> String:
        return "Hash::CRC32"

    def prepare(mut self, mut helper: Helper) raises:
        self.data = List[UInt8]()
        for _ in range(self.size):
            self.data.append(UInt8(helper.next_int(256)))

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.result += Self._crc32(self.data)

    def checksum(mut self) -> UInt32:
        return self.result

    @staticmethod
    def _crc32(data: List[UInt8]) -> UInt32:
        var crc: UInt32 = 0xFFFFFFFF

        for byte in data:
            crc = crc ^ UInt32(byte)
            for _ in range(8):
                if (crc & 1) != 0:
                    crc = (crc >> 1) ^ 0xEDB88320
                else:
                    crc = crc >> 1

        return crc ^ 0xFFFFFFFF


struct _Graph(Movable):
    var vertices: Int
    var adj: List[List[Int]]

    def __init__(out self, vertices: Int, jumps: Int, jump_len: Int):
        self.vertices = vertices
        self.adj = List[List[Int]]()
        for _ in range(vertices):
            self.adj.append(List[Int]())

    def add_edge(mut self, u: Int, v: Int):
        self.adj[u].append(v)
        self.adj[v].append(u)

    def generate_random(
        mut self, mut helper: Helper, jumps: Int, jump_len: Int
    ):
        for i in range(1, self.vertices):
            self.add_edge(i, i - 1)

        for v in range(self.vertices):
            var times = helper.next_int(jumps)
            for _ in range(times):
                var offset = helper.next_int(jump_len) - jump_len // 2
                var u = v + offset
                if u >= 0 and u < self.vertices and u != v:
                    self.add_edge(v, u)


struct GraphBFS(Benchmark, Movable):
    var graph: _Graph
    var result: UInt32
    var jumps: Int
    var jump_len: Int

    def __init__(out self, config: Config) raises:
        var vertices = config.get_i64("Graph::BFS", "vertices")
        self.jumps = config.get_i64("Graph::BFS", "jumps")
        self.jump_len = config.get_i64("Graph::BFS", "jump_len")
        self.graph = _Graph(vertices, self.jumps, self.jump_len)
        self.result = 0

    def class_name(self) -> String:
        return "Graph::BFS"

    def prepare(mut self, mut helper: Helper) raises:
        self.graph.generate_random(helper, self.jumps, self.jump_len)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var length = Self.bfs_shortest_path(
            self.graph, 0, self.graph.vertices - 1
        )
        self.result += UInt32(length)

    def checksum(mut self) -> UInt32:
        return self.result

    @staticmethod
    def bfs_shortest_path(graph: _Graph, start: Int, target: Int) -> Int:
        if start == target:
            return 0

        var visited = List[Bool](length=graph.vertices, fill=False)
        var queue = List[Tuple[Int, Int]]()
        queue.append((start, 0))
        visited[start] = True

        var q_idx = 0
        while q_idx < len(queue):
            var cur = queue[q_idx]
            q_idx += 1
            var v = cur[0]
            var dist = cur[1]

            for neighbor in graph.adj[v]:
                if neighbor == target:
                    return dist + 1
                if not visited[neighbor]:
                    visited[neighbor] = True
                    queue.append((neighbor, dist + 1))

        return -1


struct GraphDFS(Benchmark, Movable):
    var graph: _Graph
    var result: UInt32
    var jumps: Int
    var jump_len: Int

    def __init__(out self, config: Config) raises:
        var vertices = config.get_i64("Graph::DFS", "vertices")
        self.jumps = config.get_i64("Graph::DFS", "jumps")
        self.jump_len = config.get_i64("Graph::DFS", "jump_len")
        self.graph = _Graph(vertices, self.jumps, self.jump_len)
        self.result = 0

    def class_name(self) -> String:
        return "Graph::DFS"

    def prepare(mut self, mut helper: Helper) raises:
        self.graph.generate_random(helper, self.jumps, self.jump_len)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var length = Self.dfs_shortest_path(
            self.graph, 0, self.graph.vertices - 1
        )
        self.result += UInt32(length)

    def checksum(mut self) -> UInt32:
        return self.result

    @staticmethod
    def dfs_shortest_path(graph: _Graph, start: Int, target: Int) -> Int:
        if start == target:
            return 0

        var visited = List[Bool](length=graph.vertices, fill=False)
        var stack = List[Tuple[Int, Int]]()
        stack.append((start, 0))
        var best_path = 2147483647

        while len(stack) > 0:
            var last = len(stack) - 1
            var cur = stack[last]
            _ = stack.pop()
            var v = cur[0]
            var dist = cur[1]

            if visited[v] or dist >= best_path:
                continue
            visited[v] = True

            for neighbor in graph.adj[v]:
                if neighbor == target:
                    if dist + 1 < best_path:
                        best_path = dist + 1
                elif not visited[neighbor]:
                    stack.append((neighbor, dist + 1))

        if best_path == 2147483647:
            return -1
        return best_path


struct GraphAStar(Benchmark, Movable):
    var graph: _Graph
    var result: UInt32
    var jumps: Int
    var jump_len: Int

    def __init__(out self, config: Config) raises:
        var vertices = config.get_i64("Graph::AStar", "vertices")
        self.jumps = config.get_i64("Graph::AStar", "jumps")
        self.jump_len = config.get_i64("Graph::AStar", "jump_len")
        self.graph = _Graph(vertices, self.jumps, self.jump_len)
        self.result = 0

    def class_name(self) -> String:
        return "Graph::AStar"

    def prepare(mut self, mut helper: Helper) raises:
        self.graph.generate_random(helper, self.jumps, self.jump_len)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var length = Self.astar_shortest_path(
            self.graph, 0, self.graph.vertices - 1
        )
        self.result += UInt32(length)

    def checksum(mut self) -> UInt32:
        return self.result

    @staticmethod
    def astar_shortest_path(graph: _Graph, start: Int, target: Int) -> Int:
        if start == target:
            return 0

        var g_score = List[Int](length=graph.vertices, fill=2147483647)
        g_score[start] = 0

        var open_set = _PriorityQueue(graph.vertices)
        open_set.push(start, target - start)

        var in_open = List[Bool](length=graph.vertices, fill=False)
        in_open[start] = True

        var closed = List[Bool](length=graph.vertices, fill=False)

        while not open_set.empty():
            var cur = open_set.pop()
            var current = cur[0]
            closed[current] = True
            in_open[current] = False

            if current == target:
                return g_score[current]

            for neighbor in graph.adj[current]:
                if closed[neighbor]:
                    continue

                var tentative_g = g_score[current] + 1

                if tentative_g < g_score[neighbor]:
                    g_score[neighbor] = tentative_g
                    var f = tentative_g + (target - neighbor)

                    if not in_open[neighbor]:
                        open_set.push(neighbor, f)
                        in_open[neighbor] = True

        return -1


def _quick_sort(mut arr: List[Int], low: Int, high: Int):
    if low >= high:
        return

    var pivot = arr[(low + high) // 2]
    var i = low
    var j = high

    while i <= j:
        while arr[i] < pivot:
            i += 1
        while arr[j] > pivot:
            j -= 1
        if i <= j:
            var tmp = arr[i]
            arr[i] = arr[j]
            arr[j] = tmp
            i += 1
            j -= 1

    _quick_sort(arr, low, j)
    _quick_sort(arr, i, high)


def _merge_sort(mut arr: List[Int], mut temp: List[Int], left: Int, right: Int):
    if left >= right:
        return

    var mid = (left + right) // 2
    _merge_sort(arr, temp, left, mid)
    _merge_sort(arr, temp, mid + 1, right)
    _merge(arr, temp, left, mid, right)


def _merge(
    mut arr: List[Int], mut temp: List[Int], left: Int, mid: Int, right: Int
):
    for i in range(left, right + 1):
        temp[i] = arr[i]

    var i = left
    var j = mid + 1
    var k = left

    while i <= mid and j <= right:
        if temp[i] <= temp[j]:
            arr[k] = temp[i]
            i += 1
        else:
            arr[k] = temp[j]
            j += 1
        k += 1

    while i <= mid:
        arr[k] = temp[i]
        i += 1
        k += 1


struct SortQuick(Benchmark, Movable):
    var size: Int
    var data: List[Int]
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.size = config.get_i64("Sort::Quick", "size")
        self.data = List[Int]()
        self.result = 0

    def class_name(self) -> String:
        return "Sort::Quick"

    def prepare(mut self, mut helper: Helper) raises:
        self.data = List[Int]()
        for _ in range(self.size):
            self.data.append(helper.next_int(1000000))

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.result += UInt32(self.data[helper.next_int(self.size)])
        var sorted = self.data.copy()
        _quick_sort(sorted, 0, len(sorted) - 1)
        self.result += UInt32(sorted[helper.next_int(self.size)])

    def checksum(mut self) -> UInt32:
        return self.result


struct SortMerge(Benchmark, Movable):
    var size: Int
    var data: List[Int]
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.size = config.get_i64("Sort::Merge", "size")
        self.data = List[Int]()
        self.result = 0

    def class_name(self) -> String:
        return "Sort::Merge"

    def prepare(mut self, mut helper: Helper) raises:
        self.data = List[Int]()
        for _ in range(self.size):
            self.data.append(helper.next_int(1000000))

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.result += UInt32(self.data[helper.next_int(self.size)])
        var sorted = self.data.copy()
        var temp = List[Int](length=len(sorted), fill=0)
        _merge_sort(sorted, temp, 0, len(sorted) - 1)
        self.result += UInt32(sorted[helper.next_int(self.size)])

    def checksum(mut self) -> UInt32:
        return self.result


struct SortSelf(Benchmark, Movable):
    var size: Int
    var data: List[Int]
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.size = config.get_i64("Sort::Self", "size")
        self.data = List[Int]()
        self.result = 0

    def class_name(self) -> String:
        return "Sort::Self"

    def prepare(mut self, mut helper: Helper) raises:
        self.data = List[Int]()
        for _ in range(self.size):
            self.data.append(helper.next_int(1000000))

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.result += UInt32(self.data[helper.next_int(self.size)])
        var sorted = self.data.copy()

        sort(Span(sorted))

        self.result += UInt32(sorted[helper.next_int(self.size)])

    def checksum(mut self) -> UInt32:
        return self.result


struct Sieve(Benchmark, Movable):
    var limit: Int
    var checksum_: UInt32

    def __init__(out self, config: Config) raises:
        self.limit = config.get_i64("Etc::Sieve", "limit")
        self.checksum_ = 0

    def class_name(self) -> String:
        return "Etc::Sieve"

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var limit = self.limit
        var primes = List[UInt8](length=limit + 1, fill=1)
        primes[0] = 0
        primes[1] = 0

        var sqrt_limit = Int(sqrt(Float64(limit)))

        for p in range(2, sqrt_limit + 1):
            if primes[p] == 1:
                var start = p * p
                var multiple = start
                while multiple <= limit:
                    primes[multiple] = 0
                    multiple += p

        var last_prime: Int = 2
        var count: Int = 1

        var n = 3
        while n <= limit:
            if primes[n] == 1:
                last_prime = n
                count += 1
            n += 2

        self.checksum_ += UInt32(last_prime + count)

    def checksum(mut self) -> UInt32:
        return self.checksum_


struct _Vec3(Copyable, ImplicitlyCopyable, Movable):
    var x: Float64
    var y: Float64
    var z: Float64

    def __init__(out self, x: Float64, y: Float64, z: Float64):
        self.x = x
        self.y = y
        self.z = z

    def scale(self, s: Float64) -> Self:
        return Self(self.x * s, self.y * s, self.z * s)

    def __add__(self, other: Self) -> Self:
        return Self(self.x + other.x, self.y + other.y, self.z + other.z)

    def __sub__(self, other: Self) -> Self:
        return Self(self.x - other.x, self.y - other.y, self.z - other.z)

    def dot(self, other: Self) -> Float64:
        return self.x * other.x + self.y * other.y + self.z * other.z

    def magnitude(self) -> Float64:
        return sqrt(self.dot(self))

    def normalize(self) -> Self:
        var mag = self.magnitude()
        if mag == 0.0:
            return Self(0.0, 0.0, 0.0)
        return self.scale(1.0 / mag)


struct _Color(Copyable, ImplicitlyCopyable, Movable):
    var r: Float64
    var g: Float64
    var b: Float64

    def __init__(out self, r: Float64, g: Float64, b: Float64):
        self.r = r
        self.g = g
        self.b = b

    def scale(self, s: Float64) -> Self:
        return Self(self.r * s, self.g * s, self.b * s)

    def __add__(self, other: Self) -> Self:
        return Self(self.r + other.r, self.g + other.g, self.b + other.b)


struct _Sphere(Copyable, ImplicitlyCopyable, Movable):
    var center: _Vec3
    var radius: Float64
    var color: _Color

    def __init__(out self, center: _Vec3, radius: Float64, color: _Color):
        self.center = center
        self.radius = radius
        self.color = color

    def get_normal(self, pt: _Vec3) -> _Vec3:
        return (pt - self.center).normalize()


struct _Light(Copyable, ImplicitlyCopyable, Movable):
    var position: _Vec3
    var color: _Color

    def __init__(out self, position: _Vec3, color: _Color):
        self.position = position
        self.color = color


struct TextRaytracer(Benchmark, Movable):
    var w: Int
    var h: Int
    var result: UInt32
    var lut: List[Int]
    var light1: _Light
    var scene: List[_Sphere]

    def __init__(out self, config: Config) raises:
        self.w = config.get_i64("Etc::TextRaytracer", "w")
        self.h = config.get_i64("Etc::TextRaytracer", "h")
        self.result = 0

        var lut = List[Int]()
        lut.append(46)
        lut.append(45)
        lut.append(43)
        lut.append(42)
        lut.append(88)
        lut.append(77)
        self.lut = lut^

        self.light1 = _Light(_Vec3(0.7, -1.0, 1.7), _Color(1.0, 1.0, 1.0))

        self.scene = List[_Sphere]()
        self.scene.append(
            _Sphere(_Vec3(-1.0, 0.0, 3.0), 0.3, _Color(1.0, 0.0, 0.0))
        )
        self.scene.append(
            _Sphere(_Vec3(0.0, 0.0, 3.0), 0.8, _Color(0.0, 1.0, 0.0))
        )
        self.scene.append(
            _Sphere(_Vec3(1.0, 0.0, 3.0), 0.4, _Color(0.0, 0.0, 1.0))
        )

    def class_name(self) -> String:
        return "Etc::TextRaytracer"

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var res: Int = 0

        for j in range(self.h):
            for i in range(self.w):
                var fw = Float64(self.w)
                var fi = Float64(i)
                var fj = Float64(j)
                var fh = Float64(self.h)

                var ray_orig = _Vec3(0.0, 0.0, 0.0)
                var ray_dir = _Vec3(
                    (fi - fw / 2.0) / fw,
                    (fj - fh / 2.0) / fh,
                    1.0,
                ).normalize()

                var hit_obj: Optional[_Sphere] = None
                var hit_val: Float64 = 0.0

                for obj in self.scene:
                    var t = Self._intersect_sphere(
                        ray_orig, ray_dir, obj.center, obj.radius
                    )
                    if t >= 0.0 and t < 10000.0:
                        hit_obj = obj
                        hit_val = t
                        break

                if hit_obj:
                    var idx = Self._shade_pixel(
                        ray_orig, ray_dir, hit_obj[], hit_val, self.light1
                    )
                    res += self.lut[idx]
                else:
                    res += 32

        self.result += UInt32(res)

    def checksum(self) -> UInt32:
        return self.result

    @staticmethod
    def _intersect_sphere(
        ray_orig: _Vec3, ray_dir: _Vec3, center: _Vec3, radius: Float64
    ) -> Float64:
        var l = center - ray_orig
        var tca = l.dot(ray_dir)
        if tca < 0.0:
            return -1.0

        var d2 = l.dot(l) - tca * tca
        var r2 = radius * radius
        if d2 > r2:
            return -1.0

        var thc = sqrt(r2 - d2)
        var t0 = tca - thc

        if t0 > 10000.0:
            return -1.0

        return t0

    @staticmethod
    def _shade_pixel(
        ray_orig: _Vec3,
        ray_dir: _Vec3,
        obj: _Sphere,
        tval: Float64,
        light: _Light,
    ) -> Int:
        var pi = ray_orig + ray_dir.scale(tval)
        var color = Self._diffuse_shading(pi, obj, light)
        var col = (color.r + color.g + color.b) / 3.0
        var idx = Int(col * 6.0)
        if idx < 0:
            idx = 0
        if idx >= 6:
            idx = 5
        return idx

    @staticmethod
    def _diffuse_shading(pi: _Vec3, obj: _Sphere, light: _Light) -> _Color:
        var n = obj.get_normal(pi)
        var light_dir = (light.position - pi).normalize()
        var lam1 = light_dir.dot(n)
        var lam2: Float64
        if lam1 < 0.0:
            lam2 = 0.0
        elif lam1 > 1.0:
            lam2 = 1.0
        else:
            lam2 = lam1
        return light.color.scale(lam2 * 0.5) + obj.color.scale(0.3)


comptime NN_LEARNING_RATE: Float64 = 1.0
comptime NN_MOMENTUM: Float64 = 0.3
comptime NN_TRAIN_RATE: Float64 = 0.3


struct _Synapse(Copyable, ImplicitlyCopyable, Movable):
    var weight: Float64
    var prev_weight: Float64
    var source_idx: Int
    var dest_idx: Int

    def __init__(out self, source_idx: Int, dest_idx: Int, mut helper: Helper):
        var r = helper.next_float(1.0)
        self.weight = r * 2.0 - 1.0
        self.prev_weight = self.weight
        self.source_idx = source_idx
        self.dest_idx = dest_idx


struct _Neuron(Copyable, Movable):
    var threshold: Float64
    var prev_threshold: Float64
    var output: Float64
    var error: Float64
    var synapses_in: List[Int]
    var synapses_out: List[Int]

    def __init__(out self, mut helper: Helper):
        var r = helper.next_float(1.0)
        self.threshold = r * 2.0 - 1.0
        self.prev_threshold = self.threshold
        self.output = 0.0
        self.error = 0.0
        self.synapses_in = List[Int]()
        self.synapses_out = List[Int]()

    def derivative(self) -> Float64:
        return self.output * (1.0 - self.output)


struct _NN(Movable):
    var neurons: List[_Neuron]
    var input_indices: List[Int]
    var hidden_indices: List[Int]
    var output_indices: List[Int]
    var synapses: List[_Synapse]

    def __init__(
        out self, inputs: Int, hidden: Int, outputs: Int, mut helper: Helper
    ):
        var total = inputs + hidden + outputs

        self.neurons = List[_Neuron]()
        for _ in range(total):
            self.neurons.append(_Neuron(helper))

        self.input_indices = List[Int]()
        for i in range(inputs):
            self.input_indices.append(i)

        self.hidden_indices = List[Int]()
        for i in range(hidden):
            self.hidden_indices.append(inputs + i)

        self.output_indices = List[Int]()
        for i in range(outputs):
            self.output_indices.append(inputs + hidden + i)

        self.synapses = List[_Synapse]()
        for i in range(inputs):
            var src_idx = self.input_indices[i]
            for j in range(hidden):
                var dst_idx = self.hidden_indices[j]
                var syn_idx = len(self.synapses)
                self.synapses.append(_Synapse(src_idx, dst_idx, helper))
                self.neurons[src_idx].synapses_out.append(syn_idx)
                self.neurons[dst_idx].synapses_in.append(syn_idx)

        for i in range(hidden):
            var src_idx = self.hidden_indices[i]
            for j in range(outputs):
                var dst_idx = self.output_indices[j]
                var syn_idx = len(self.synapses)
                self.synapses.append(_Synapse(src_idx, dst_idx, helper))
                self.neurons[src_idx].synapses_out.append(syn_idx)
                self.neurons[dst_idx].synapses_in.append(syn_idx)

    def feed_forward(mut self, inputs: List[Float64]):
        for i in range(len(inputs)):
            self.neurons[self.input_indices[i]].output = inputs[i]

        for i in range(len(self.hidden_indices)):
            var neuron_idx = self.hidden_indices[i]
            var activation: Float64 = 0.0
            for syn_idx in self.neurons[neuron_idx].synapses_in:
                var syn = self.synapses[syn_idx]
                activation += syn.weight * self.neurons[syn.source_idx].output
            activation -= self.neurons[neuron_idx].threshold
            self.neurons[neuron_idx].output = 1.0 / (1.0 + exp(-activation))

        for i in range(len(self.output_indices)):
            var neuron_idx = self.output_indices[i]
            var activation: Float64 = 0.0
            for syn_idx in self.neurons[neuron_idx].synapses_in:
                var syn = self.synapses[syn_idx]
                activation += syn.weight * self.neurons[syn.source_idx].output
            activation -= self.neurons[neuron_idx].threshold
            self.neurons[neuron_idx].output = 1.0 / (1.0 + exp(-activation))

    def train(mut self, inputs: List[Float64], targets: List[Float64]):
        self.feed_forward(inputs)

        for i in range(len(self.output_indices)):
            var neuron_idx = self.output_indices[i]
            ref neuron = self.neurons[neuron_idx]
            neuron.error = (targets[i] - neuron.output) * neuron.derivative()
            self._update_weights(neuron_idx)

        for i in range(len(self.hidden_indices)):
            var neuron_idx = self.hidden_indices[i]
            ref neuron = self.neurons[neuron_idx]
            var sum: Float64 = 0.0
            for syn_idx in neuron.synapses_out:
                var syn = self.synapses[syn_idx]
                sum += syn.prev_weight * self.neurons[syn.dest_idx].error
            neuron.error = sum * neuron.derivative()
            self._update_weights(neuron_idx)

    def _update_weights(mut self, neuron_idx: Int):
        ref neuron = self.neurons[neuron_idx]

        for syn_idx in neuron.synapses_in:
            var syn = self.synapses[syn_idx]
            var temp_weight = syn.weight
            syn.weight += (
                NN_TRAIN_RATE
                * NN_LEARNING_RATE
                * neuron.error
                * self.neurons[syn.source_idx].output
            )
            syn.weight += NN_MOMENTUM * (syn.weight - syn.prev_weight)
            syn.prev_weight = temp_weight
            self.synapses[syn_idx] = syn

        var temp_threshold = neuron.threshold
        neuron.threshold += (
            NN_TRAIN_RATE * NN_LEARNING_RATE * neuron.error * (-1.0)
        )
        neuron.threshold += NN_MOMENTUM * (
            neuron.threshold - neuron.prev_threshold
        )
        neuron.prev_threshold = temp_threshold


struct NeuralNet(Benchmark, Movable):
    var nn: _NN
    var input00: List[Float64]
    var input10: List[Float64]
    var input01: List[Float64]
    var input11: List[Float64]
    var target0: List[Float64]
    var target1: List[Float64]

    def __init__(out self, config: Config) raises:
        var dummy_helper = Helper()
        self.nn = _NN(0, 0, 0, dummy_helper)

        self.input00 = List[Float64]()
        self.input00.append(0.0)
        self.input00.append(0.0)

        self.input10 = List[Float64]()
        self.input10.append(1.0)
        self.input10.append(0.0)

        self.input01 = List[Float64]()
        self.input01.append(0.0)
        self.input01.append(1.0)

        self.input11 = List[Float64]()
        self.input11.append(1.0)
        self.input11.append(1.0)

        self.target0 = List[Float64]()
        self.target0.append(0.0)

        self.target1 = List[Float64]()
        self.target1.append(1.0)

    def class_name(self) -> String:
        return "Etc::NeuralNet"

    def prepare(mut self, mut helper: Helper) raises:
        helper.reset()
        self.nn = _NN(2, 10, 1, helper)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        for _ in range(1000):
            self.nn.train(self.input00, self.target0)
            self.nn.train(self.input10, self.target1)
            self.nn.train(self.input01, self.target1)
            self.nn.train(self.input11, self.target0)

    def checksum(mut self) -> UInt32:
        var sum: Float64 = 0.0

        self.nn.feed_forward(self.input00)
        sum += self.nn.neurons[self.nn.output_indices[0]].output

        self.nn.feed_forward(self.input01)
        sum += self.nn.neurons[self.nn.output_indices[0]].output

        self.nn.feed_forward(self.input10)
        sum += self.nn.neurons[self.nn.output_indices[0]].output

        self.nn.feed_forward(self.input11)
        sum += self.nn.neurons[self.nn.output_indices[0]].output

        return Helper.checksum_f64(sum)


struct _LRUNode(Copyable, Movable):
    var key: String
    var value: String
    var prev: Optional[Pointer[_LRUNode, MutUntrackedOrigin]]
    var next: Optional[Pointer[_LRUNode, MutUntrackedOrigin]]

    def __init__(out self, var key: String, var value: String):
        self.key = key^
        self.value = value^
        self.prev = Optional[Pointer[_LRUNode, MutUntrackedOrigin]](None)
        self.next = Optional[Pointer[_LRUNode, MutUntrackedOrigin]](None)

    def __deinit__(deinit self):
        pass


struct _LRUCache(Movable):
    var capacity: Int
    var cache: Dict[String, Pointer[_LRUNode, MutUntrackedOrigin]]
    var head: Optional[Pointer[_LRUNode, MutUntrackedOrigin]]
    var tail: Optional[Pointer[_LRUNode, MutUntrackedOrigin]]
    var size_val: Int

    def __init__(out self, capacity: Int):
        self.capacity = capacity
        self.cache = Dict[String, Pointer[_LRUNode, MutUntrackedOrigin]]()
        self.head = Optional[Pointer[_LRUNode, MutUntrackedOrigin]](None)
        self.tail = Optional[Pointer[_LRUNode, MutUntrackedOrigin]](None)
        self.size_val = 0

    def __deinit__(deinit self):
        var current = self.head
        while current:
            var next = current.value()[].next
            var ptr = current.value()
            ptr.unsafe_deinit_pointee()
            ptr.unsafe_free()
            current = next

    def move_to_front(
        mut self, node_ptr: Pointer[_LRUNode, MutUntrackedOrigin]
    ):
        if self.head and self.head.value() == node_ptr:
            return

        if node_ptr[].prev:
            node_ptr[].prev.value()[].next = node_ptr[].next
        if node_ptr[].next:
            node_ptr[].next.value()[].prev = node_ptr[].prev

        if self.tail and self.tail.value() == node_ptr:
            self.tail = node_ptr[].prev

        node_ptr[].prev = Optional[Pointer[_LRUNode, MutUntrackedOrigin]](None)
        node_ptr[].next = self.head

        if self.head:
            self.head.value()[].prev = Optional[
                Pointer[_LRUNode, MutUntrackedOrigin]
            ](node_ptr)

        self.head = Optional[Pointer[_LRUNode, MutUntrackedOrigin]](node_ptr)

        if not self.tail:
            self.tail = Optional[Pointer[_LRUNode, MutUntrackedOrigin]](
                node_ptr
            )

    def add_to_front(mut self, node_ptr: Pointer[_LRUNode, MutUntrackedOrigin]):
        node_ptr[].next = self.head

        if self.head:
            self.head.value()[].prev = Optional[
                Pointer[_LRUNode, MutUntrackedOrigin]
            ](node_ptr)

        self.head = Optional[Pointer[_LRUNode, MutUntrackedOrigin]](node_ptr)

        if not self.tail:
            self.tail = Optional[Pointer[_LRUNode, MutUntrackedOrigin]](
                node_ptr
            )

    def remove_oldest(mut self) raises:
        if not self.tail:
            return

        var tail_ptr = self.tail.value()

        _ = self.cache.pop(tail_ptr[].key)

        if tail_ptr[].prev:
            tail_ptr[].prev.value()[].next = Optional[
                Pointer[_LRUNode, MutUntrackedOrigin]
            ](None)

        self.tail = tail_ptr[].prev

        if self.head and self.head.value() == tail_ptr:
            self.head = Optional[Pointer[_LRUNode, MutUntrackedOrigin]](None)

        tail_ptr.unsafe_deinit_pointee()
        tail_ptr.unsafe_free()

        self.size_val -= 1

    def get(mut self, key: String) raises -> Optional[String]:
        var it = self.cache.get(key)
        if not it:
            return Optional[String](None)

        var node_ptr = it[]
        self.move_to_front(node_ptr)
        return Optional[String](node_ptr[].value)

    def put(mut self, var key: String, var value: String) raises:
        var it = self.cache.get(key)
        if it:
            var node_ptr = it[]
            node_ptr[].value = value^
            self.move_to_front(node_ptr)
            return

        if self.size_val >= self.capacity:
            self.remove_oldest()

        var node_ptr = unsafe_alloc[_LRUNode](1)
        node_ptr.unsafe_write(_LRUNode(key^, value^))

        self.cache[node_ptr[].key] = node_ptr
        self.add_to_front(node_ptr)
        self.size_val += 1

    def count(self) -> Int:
        return self.size_val


struct CacheSimulation(Benchmark, Movable):
    var values_size: Int
    var cache_size: Int
    var cache: _LRUCache
    var hits: UInt32
    var misses: UInt32
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.values_size = config.get_i64("Etc::CacheSimulation", "values")
        self.cache_size = config.get_i64("Etc::CacheSimulation", "size")
        self.cache = _LRUCache(self.cache_size)
        self.hits = 0
        self.misses = 0
        self.result = 5432

    def class_name(self) -> String:
        return "Etc::CacheSimulation"

    def prepare(mut self, mut helper: Helper) raises:
        self.cache = _LRUCache(self.cache_size)
        self.hits = 0
        self.misses = 0

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        for _ in range(1000):
            var key = String("item_", helper.next_int(self.values_size))

            var value = self.cache.get(key)
            if value:
                self.hits += 1
                self.cache.put(key, String("updated_", iteration_id))
            else:
                self.misses += 1
                self.cache.put(key, String("new_", iteration_id))

    def checksum(self) -> UInt32:
        var r = self.result
        r = (r << 5) + self.hits
        r = (r << 5) + self.misses
        r = (r << 5) + UInt32(self.cache.count())
        return r


struct _GOLCell(Copyable, Movable):
    var alive: Bool
    var next_state: Bool
    var neighbors: List[Pointer[_GOLCell, MutUntrackedOrigin]]

    def __init__(out self, alive: Bool = False):
        self.alive = alive
        self.next_state = False
        self.neighbors = List[Pointer[_GOLCell, MutUntrackedOrigin]]()

    def add_neighbor(mut self, cell: Pointer[_GOLCell, MutUntrackedOrigin]):
        self.neighbors.append(cell)

    def compute_next_state(mut self):
        var alive_neighbors = 0
        for neighbor in self.neighbors:
            if neighbor[].alive:
                alive_neighbors += 1

        if self.alive:
            self.next_state = alive_neighbors == 2 or alive_neighbors == 3
        else:
            self.next_state = alive_neighbors == 3

    def update(mut self):
        self.alive = self.next_state


struct _GOLGrid(Movable):
    var width: Int
    var height: Int
    var cells: List[List[_GOLCell]]

    def __init__(out self, width: Int, height: Int):
        self.width = width
        self.height = height
        self.cells = List[List[_GOLCell]]()

        for _ in range(height):
            var row = List[_GOLCell]()
            for _ in range(width):
                row.append(_GOLCell(False))
            self.cells.append(row^)

        self._link_neighbors()

    def _link_neighbors(mut self):
        var cell_ptrs = List[List[Pointer[_GOLCell, MutUntrackedOrigin]]]()
        for y in range(self.height):
            var row_ptrs = List[Pointer[_GOLCell, MutUntrackedOrigin]]()
            for x in range(self.width):
                row_ptrs.append(
                    Pointer(to=self.cells[y][x]).unsafe_origin_cast[
                        MutUntrackedOrigin
                    ]()
                )
            cell_ptrs.append(row_ptrs^)

        for y in range(self.height):
            for x in range(self.width):
                ref cell = self.cells[y][x]

                for dy in range(-1, 2):
                    for dx in range(-1, 2):
                        if dx == 0 and dy == 0:
                            continue

                        var ny = (y + dy + self.height) % self.height
                        var nx = (x + dx + self.width) % self.width

                        cell.add_neighbor(cell_ptrs[ny][nx])

    def next_generation(mut self):
        for y in range(self.height):
            for x in range(self.width):
                self.cells[y][x].compute_next_state()

        for y in range(self.height):
            for x in range(self.width):
                self.cells[y][x].update()

    def count_alive(self) -> Int:
        var count = 0
        for y in range(self.height):
            for x in range(self.width):
                if self.cells[y][x].alive:
                    count += 1
        return count

    def compute_hash(self) -> UInt32:
        comptime FNV_OFFSET: UInt32 = 2166136261
        comptime FNV_PRIME: UInt32 = 16777619
        var hash = FNV_OFFSET

        for y in range(self.height):
            for x in range(self.width):
                var alive_val: UInt32 = 1 if self.cells[y][x].alive else 0
                hash = (hash ^ alive_val) * FNV_PRIME

        return hash


struct GameOfLife(Benchmark, Movable):
    var w: Int
    var h: Int
    var grid: _GOLGrid

    def __init__(out self, config: Config) raises:
        self.w = config.get_i64("Etc::GameOfLife", "w")
        self.h = config.get_i64("Etc::GameOfLife", "h")
        self.grid = _GOLGrid(self.w, self.h)

    def class_name(self) -> String:
        return "Etc::GameOfLife"

    def prepare(mut self, mut helper: Helper) raises:
        for y in range(self.h):
            for x in range(self.w):
                if helper.next_float(1.0) < 0.1:
                    self.grid.cells[y][x].alive = True

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.grid.next_generation()

    def checksum(mut self) -> UInt32:
        return self.grid.compute_hash() + UInt32(self.grid.count_alive())


struct Words(Benchmark, Movable):
    var words: Int
    var word_len: Int
    var text: String
    var checksum_: UInt32

    def __init__(out self, config: Config) raises:
        self.words = config.get_i64("Etc::Words", "words")
        self.word_len = config.get_i64("Etc::Words", "word_len")
        self.text = ""
        self.checksum_ = 0

    def class_name(self) -> String:
        return "Etc::Words"

    def prepare(mut self, mut helper: Helper) raises:
        var chars = List[String]()
        for c in range(26):
            chars.append(String(chr(97 + c)))

        self.text = ""
        for i in range(self.words):
            var w_len = helper.next_int(self.word_len) + helper.next_int(3) + 3
            for _ in range(w_len):
                self.text += chars[helper.next_int(len(chars))]
            if i != self.words - 1:
                self.text += " "

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var frequencies = Dict[String, Int]()

        for word in self.text.split(" "):
            if word.byte_length() > 0:
                var word_str = String(word)
                frequencies[word_str] = frequencies.get(word_str).or_else(0) + 1

        var max_word = ""
        var max_count: Int = 0
        for item in frequencies.items():
            if item.value > max_count:
                max_count = item.value
                max_word = item.key

        self.checksum_ += UInt32(max_count)
        self.checksum_ += Helper.checksum_string(max_word)
        self.checksum_ += UInt32(len(frequencies))

    def checksum(mut self) -> UInt32:
        return self.checksum_


comptime CALC_NUMBER = 0
comptime CALC_VARIABLE = 1
comptime CALC_BINARY = 2
comptime CALC_ASSIGN = 3


struct _CalcNode(Copyable, ImplicitlyCopyable, Movable):
    var kind: Int
    var value: Int
    var name: String
    var op: String
    var left: Int
    var right: Int

    def __init__(out self, kind: Int):
        self.kind = kind
        self.value = 0
        self.name = ""
        self.op = ""
        self.left = -1
        self.right = -1


struct _CalcParser(Movable):
    var input: String
    var pos: Int
    var length: Int
    var nodes: List[_CalcNode]
    var expressions: List[Int]

    def __init__(out self, text: String):
        self.input = text
        self.pos = 0
        self.length = text.byte_length()
        self.nodes = List[_CalcNode]()
        self.expressions = List[Int]()

    def parse(mut self):
        while self.pos < self.length:
            self._skip_whitespace()
            if self.pos >= self.length:
                break
            self.expressions.append(self._parse_expression())
            self._skip_whitespace()
            while self.pos < self.length and (
                String(self.input[byte=self.pos]) == "\n"
                or String(self.input[byte=self.pos]) == ";"
            ):
                self.pos += 1
                self._skip_whitespace()

    def _parse_expression(mut self) -> Int:
        var node_idx = self._parse_term()
        return self._parse_expression_rest(node_idx)

    def _parse_expression_rest(mut self, mut node_idx: Int) -> Int:
        while self.pos < self.length:
            self._skip_whitespace()
            if self.pos >= self.length:
                break

            var ch = String(self.input[byte=self.pos])
            if ch == "+" or ch == "-":
                var op = ch
                self.pos += 1
                var right_idx = self._parse_term()
                var new_node = _CalcNode(CALC_BINARY)
                new_node.op = op
                new_node.left = node_idx
                new_node.right = right_idx
                self.nodes.append(new_node)
                node_idx = len(self.nodes) - 1
            else:
                break

        return node_idx

    def _parse_term(mut self) -> Int:
        var node_idx = self._parse_factor()
        return self._parse_term_rest(node_idx)

    def _parse_term_rest(mut self, mut node_idx: Int) -> Int:
        while self.pos < self.length:
            self._skip_whitespace()
            if self.pos >= self.length:
                break

            var ch = String(self.input[byte=self.pos])
            if ch == "*" or ch == "/" or ch == "%":
                var op = ch
                self.pos += 1
                var right_idx = self._parse_factor()
                var new_node = _CalcNode(CALC_BINARY)
                new_node.op = op
                new_node.left = node_idx
                new_node.right = right_idx
                self.nodes.append(new_node)
                node_idx = len(self.nodes) - 1
            else:
                break

        return node_idx

    def _parse_factor(mut self) -> Int:
        self._skip_whitespace()
        if self.pos >= self.length:
            return self._add_number(0)

        var ch = String(self.input[byte=self.pos])

        if ch >= "0" and ch <= "9":
            return self._parse_number()
        elif (ch >= "a" and ch <= "z") or (ch >= "A" and ch <= "Z"):
            return self._parse_variable()
        elif ch == "(":
            self.pos += 1
            var node_idx = self._parse_expression()
            self._skip_whitespace()
            if (
                self.pos < self.length
                and String(self.input[byte=self.pos]) == ")"
            ):
                self.pos += 1
            return node_idx

        return self._add_number(0)

    def _parse_number(mut self) -> Int:
        var v: Int = 0
        while self.pos < self.length:
            var ch = String(self.input[byte=self.pos])
            if ch >= "0" and ch <= "9":
                v = v * 10 + (ord(ch) - 48)
                self.pos += 1
            else:
                break
        return self._add_number(v)

    def _parse_variable(mut self) -> Int:
        var start = self.pos
        while self.pos < self.length:
            var ch = String(self.input[byte=self.pos])
            if (
                (ch >= "a" and ch <= "z")
                or (ch >= "A" and ch <= "Z")
                or (ch >= "0" and ch <= "9")
            ):
                self.pos += 1
            else:
                break

        var var_name = ""
        for i in range(start, self.pos):
            var_name += String(self.input[byte=i])

        self._skip_whitespace()
        if self.pos < self.length and String(self.input[byte=self.pos]) == "=":
            self.pos += 1
            var expr = self._parse_expression()
            var node = _CalcNode(CALC_ASSIGN)
            node.name = var_name
            node.left = expr
            self.nodes.append(node)
            return len(self.nodes) - 1

        var node = _CalcNode(CALC_VARIABLE)
        node.name = var_name
        self.nodes.append(node)
        return len(self.nodes) - 1

    def _add_number(mut self, v: Int) -> Int:
        var node = _CalcNode(CALC_NUMBER)
        node.value = v
        self.nodes.append(node)
        return len(self.nodes) - 1

    def _skip_whitespace(mut self):
        while self.pos < self.length:
            var ch = String(self.input[byte=self.pos])
            if ch == " " or ch == "\t" or ch == "\n" or ch == "\r":
                self.pos += 1
            else:
                break


struct CalculatorAst(Benchmark, Movable):
    var n: Int
    var text: String
    var parser: _CalcParser
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.n = config.get_i64("Calculator::Ast", "operations")
        self.text = ""
        self.parser = _CalcParser("")
        self.result = 0

    def class_name(self) -> String:
        return "Calculator::Ast"

    def prepare(mut self, mut helper: Helper) raises:
        self.text = Self._generate_random_program(self.n, helper)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.parser = _CalcParser(self.text)
        self.parser.parse()
        self.result += UInt32(len(self.parser.expressions))

        if len(self.parser.expressions) > 0:
            var last_idx = self.parser.expressions[
                len(self.parser.expressions) - 1
            ]
            var last_node = self.parser.nodes[last_idx]
            if last_node.kind == CALC_ASSIGN:
                self.result += Helper.checksum_string(last_node.name)

    def checksum(self) -> UInt32:
        return self.result

    @staticmethod
    def _generate_random_program(n: Int, mut helper: Helper) -> String:
        var result = "v0 = 1\n"

        for i in range(1, 11):
            var v = i
            result += String("v", v, " = v", v - 1, " + ", v, "\n")

        for i in range(n):
            var v = i + 10
            result += String("v", v, " = v", v - 1, " + ")

            var r = helper.next_int(10)
            if r == 0:
                result += String(
                    "(v",
                    v - 1,
                    " / 3) * 4 - ",
                    i,
                    " / (3 + (18 - v",
                    v - 2,
                    ")) % v",
                    v - 3,
                    " + 2 * ((9 - v",
                    v - 6,
                    ") * (v",
                    v - 5,
                    " + 7))",
                )
            elif r == 1:
                result += String(
                    "v",
                    v - 1,
                    " + (v",
                    v - 2,
                    " + v",
                    v - 3,
                    ") * v",
                    v - 4,
                    " - (v",
                    v - 5,
                    " /  v",
                    v - 6,
                    ")",
                )
            elif r == 2:
                result += String("(3789 - (((v", v - 7, ")))) + 1")
            elif r == 3:
                result += String("4/2 * (1-3) + v", v - 9, "/v", v - 5)
            elif r == 4:
                result += String("1+2+3+4+5+6+v", v - 1)
            elif r == 5:
                result += String("(99999 / v", v - 3, ")")
            elif r == 6:
                result += String("0 + 0 - v", v - 8)
            elif r == 7:
                result += String("((((((((((v", v - 6, ")))))))))) * 2")
            elif r == 8:
                result += String(i, " * (v", v - 1, "%6)%7")
            elif r == 9:
                result += String("(1)/(0-v", v - 5, ") + (v", v - 7, ")")

            result += "\n"

        return result


struct CalculatorInterpreter(Benchmark, Movable):
    var n: Int
    var parser: _CalcParser
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.n = config.get_i64("Calculator::Interpreter", "operations")
        self.parser = _CalcParser("")
        self.result = 0

    def class_name(self) -> String:
        return "Calculator::Interpreter"

    def prepare(mut self, mut helper: Helper) raises:
        var text = CalculatorAst._generate_random_program(self.n, helper)
        self.parser = _CalcParser(text)
        self.parser.parse()

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var variables = Dict[String, Int]()

        var result: Int = 0
        for expr_idx in self.parser.expressions:
            result = self._evaluate(expr_idx, variables)

        self.result += UInt32(result)

    def checksum(self) -> UInt32:
        return self.result

    @staticmethod
    def _simple_div(a: Int, b: Int) -> Int:
        if b == 0:
            return 0
        if (a >= 0 and b > 0) or (a < 0 and b < 0):
            return a // b
        else:
            var abs_a = a if a >= 0 else -a
            var abs_b = b if b >= 0 else -b
            return -(abs_a // abs_b)

    @staticmethod
    def _simple_mod(a: Int, b: Int) -> Int:
        if b == 0:
            return 0
        return a - Self._simple_div(a, b) * b

    def _evaluate(
        mut self, node_idx: Int, mut variables: Dict[String, Int]
    ) -> Int:
        var node = self.parser.nodes[node_idx]

        if node.kind == CALC_NUMBER:
            return node.value
        elif node.kind == CALC_VARIABLE:
            return variables.get(node.name).or_else(0)
        elif node.kind == CALC_BINARY:
            var left = self._evaluate(node.left, variables)
            var right = self._evaluate(node.right, variables)
            if node.op == "+":
                return left + right
            elif node.op == "-":
                return left - right
            elif node.op == "*":
                return left * right
            elif node.op == "/":
                return Self._simple_div(left, right)
            elif node.op == "%":
                return Self._simple_mod(left, right)
            return 0
        elif node.kind == CALC_ASSIGN:
            var value = self._evaluate(node.left, variables)
            variables[node.name] = value
            return value

        return 0


def generate_test_data(size: Int) -> List[UInt8]:
    var pattern = "ABRACADABRA"
    var data = List[UInt8]()
    var pl = pattern.byte_length()
    for i in range(size):
        data.append(pattern.as_bytes()[i % pl])
    return data^


struct BWTResult(Copyable, Movable):
    var transformed: List[UInt8]
    var original_idx: Int

    def __init__(out self):
        self.transformed = List[UInt8]()
        self.original_idx = 0

    def __init__(out self, var transformed: List[UInt8], original_idx: Int):
        self.transformed = transformed^
        self.original_idx = original_idx


struct BWTEncode(Benchmark, Movable):
    var size: Int
    var result: UInt32
    var test_data: List[UInt8]
    var bwt_result: BWTResult

    def __init__(out self, config: Config) raises:
        self.size = config.get_i64("Compress::BWTEncode", "size")
        self.result = 0
        self.test_data = List[UInt8]()
        self.bwt_result = BWTResult()

    def class_name(self) -> String:
        return "Compress::BWTEncode"

    def prepare(mut self, mut helper: Helper) raises:
        self.test_data = generate_test_data(self.size)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.bwt_result = Self._bwt_transform(self.test_data)
        self.result += UInt32(len(self.bwt_result.transformed))

    def checksum(mut self) -> UInt32:
        return self.result

    @staticmethod
    def _bwt_transform(input: List[UInt8]) -> BWTResult:
        var n = len(input)
        if n == 0:
            return BWTResult()

        var counts = List[Int](length=256, fill=0)
        for i in range(n):
            counts[Int(input[i])] += 1

        var positions = List[Int](length=256, fill=0)
        var total = 0
        for i in range(256):
            positions[i] = total
            total += counts[i]

        var sa = List[Int](length=n, fill=0)
        var temp_counts = List[Int](length=256, fill=0)
        for i in range(n):
            var byte = Int(input[i])
            var pos = positions[byte] + temp_counts[byte]
            sa[pos] = i
            temp_counts[byte] += 1

        if n > 1:
            var rank = List[Int](length=n, fill=0)
            var current_rank = 0
            var prev_char = input[sa[0]]

            for i in range(n):
                var idx = sa[i]
                if input[idx] != prev_char:
                    current_rank += 1
                    prev_char = input[idx]
                rank[idx] = current_rank

            var k = 1
            while k < n:
                var rank2 = List[Int](length=n, fill=0)
                for i in range(n):
                    rank2[i] = rank[(i + k) % n]

                sort(
                    Span(sa),
                    lambda (a: Int, b: Int) -> Bool: (
                        rank[a]
                        < rank[b] if rank[a]
                        != rank[b] else rank2[a]
                        < rank2[b]
                    ),
                )

                var new_rank = List[Int](length=n, fill=0)
                new_rank[sa[0]] = 0
                for i in range(1, n):
                    var prev = sa[i - 1]
                    var curr = sa[i]
                    if rank[prev] != rank[curr] or rank2[prev] != rank2[curr]:
                        new_rank[curr] = new_rank[prev] + 1
                    else:
                        new_rank[curr] = new_rank[prev]

                rank = new_rank^
                k <<= 1

        var transformed = List[UInt8](length=n, fill=0)
        var original_idx = 0

        for i in range(n):
            var suffix = sa[i]
            if suffix == 0:
                transformed[i] = input[n - 1]
                original_idx = i
            else:
                transformed[i] = input[suffix - 1]

        return BWTResult(transformed^, original_idx)


struct BWTDecode(Benchmark, Movable):
    var size: Int
    var result: UInt32
    var test_data: List[UInt8]
    var inverted: List[UInt8]
    var bwt_result: BWTResult

    def __init__(out self, config: Config) raises:
        self.size = config.get_i64("Compress::BWTDecode", "size")
        self.result = 0
        self.test_data = List[UInt8]()
        self.inverted = List[UInt8]()
        self.bwt_result = BWTResult()

    def class_name(self) -> String:
        return "Compress::BWTDecode"

    def prepare(mut self, mut helper: Helper) raises:
        self.test_data = generate_test_data(self.size)
        self.bwt_result = BWTEncode._bwt_transform(self.test_data)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.inverted = Self._bwt_inverse(self.bwt_result)
        self.result += UInt32(len(self.inverted))

    def checksum(mut self) -> UInt32:
        if len(self.inverted) == len(self.test_data):
            var equal = True
            for i in range(len(self.test_data)):
                if self.inverted[i] != self.test_data[i]:
                    equal = False
                    break
            if equal:
                self.result += 100000
        return self.result

    @staticmethod
    def _bwt_inverse(bwt_result: BWTResult) -> List[UInt8]:
        ref bwt = bwt_result.transformed
        var n = len(bwt)
        if n == 0:
            return List[UInt8]()

        var counts = List[Int](length=256, fill=0)
        for i in range(n):
            counts[Int(bwt[i])] += 1

        var positions = List[Int](length=256, fill=0)
        var total = 0
        for i in range(256):
            positions[i] = total
            total += counts[i]

        var next_arr = List[Int](length=n, fill=0)
        var temp_counts = List[Int](length=256, fill=0)

        for i in range(n):
            var byte = Int(bwt[i])
            var pos = positions[byte] + temp_counts[byte]
            next_arr[pos] = i
            temp_counts[byte] += 1

        var result = List[UInt8](length=n, fill=0)
        var idx = bwt_result.original_idx

        for i in range(n):
            idx = next_arr[idx]
            result[i] = bwt[idx]

        return result^


struct _HuffmanNode(Copyable, ImplicitlyCopyable, Movable):
    var frequency: Int
    var byte_val: UInt8
    var is_leaf: Bool
    var left: Int
    var right: Int

    def __init__(out self, frequency: Int, byte_val: UInt8, is_leaf: Bool):
        self.frequency = frequency
        self.byte_val = byte_val
        self.is_leaf = is_leaf
        self.left = -1
        self.right = -1


struct _HuffmanCodes(Copyable, Movable):
    var code_lengths: List[Int]
    var codes: List[Int]

    def __init__(out self):
        self.code_lengths = List[Int](length=256, fill=0)
        self.codes = List[Int](length=256, fill=0)


struct _HuffEncodedResult(Copyable, Movable):
    var frequencies: List[Int]
    var data: List[UInt8]
    var bit_count: Int

    def __init__(out self):
        self.frequencies = List[Int]()
        self.data = List[UInt8]()
        self.bit_count = 0

    def __init__(
        out self,
        var data: List[UInt8],
        bit_count: Int,
        var frequencies: List[Int],
    ):
        self.data = data^
        self.bit_count = bit_count
        self.frequencies = frequencies^


def _build_huffman_tree_nodes(
    frequencies: List[Int],
) -> Tuple[List[_HuffmanNode], Int]:
    var nodes = List[_HuffmanNode]()
    var heap = List[Int]()

    for i in range(256):
        if frequencies[i] > 0:
            nodes.append(_HuffmanNode(frequencies[i], UInt8(i), True))
            heap.append(len(nodes) - 1)

    sort(
        Span(heap),
        lambda (a: Int, b: Int) -> Bool: nodes[a].frequency
        < nodes[b].frequency,
    )

    if len(heap) == 1:
        var leaf_idx = heap[0]
        nodes.append(_HuffmanNode(nodes[leaf_idx].frequency, 0, False))
        var root_idx = len(nodes) - 1
        nodes[root_idx].left = leaf_idx
        nodes.append(_HuffmanNode(0, 0, True))
        nodes[root_idx].right = len(nodes) - 1
        return (nodes^, root_idx)

    while len(heap) > 1:
        var left_idx = heap[0]
        _ = heap.pop(0)
        var right_idx = heap[0]
        _ = heap.pop(0)

        nodes.append(
            _HuffmanNode(
                nodes[left_idx].frequency + nodes[right_idx].frequency, 0, False
            )
        )
        var parent_idx = len(nodes) - 1
        nodes[parent_idx].left = left_idx
        nodes[parent_idx].right = right_idx

        var lo = 0
        var hi = len(heap)
        while lo < hi:
            var mid = (lo + hi) // 2
            if nodes[heap[mid]].frequency < nodes[parent_idx].frequency:
                lo = mid + 1
            else:
                hi = mid
        heap.insert(lo, parent_idx)

    return (nodes^, heap[0])


def _build_huffman_codes(
    ref nodes: List[_HuffmanNode],
    node_idx: Int,
    code: Int,
    length: Int,
    mut codes: _HuffmanCodes,
):
    var node = nodes[node_idx]
    if node.is_leaf:
        if length > 0 or node.byte_val != 0:
            var idx = Int(node.byte_val)
            codes.code_lengths[idx] = length
            codes.codes[idx] = code
    else:
        if node.left >= 0:
            _build_huffman_codes(nodes, node.left, code << 1, length + 1, codes)
        if node.right >= 0:
            _build_huffman_codes(
                nodes, node.right, (code << 1) | 1, length + 1, codes
            )


def _huffman_encode(
    data: List[UInt8], codes: _HuffmanCodes, var frequencies: List[Int]
) -> _HuffEncodedResult:
    var result = List[UInt8]()
    var current_byte: UInt8 = 0
    var bit_pos: Int = 0
    var total_bits: Int = 0

    for i in range(len(data)):
        var byte = Int(data[i])
        var code = codes.codes[byte]
        var length = codes.code_lengths[byte]

        for j in range(length):
            var bit_idx = length - 1 - j
            if (code & (1 << bit_idx)) != 0:
                current_byte |= UInt8(1 << (7 - bit_pos))
            bit_pos += 1
            total_bits += 1

            if bit_pos == 8:
                result.append(current_byte)
                current_byte = 0
                bit_pos = 0

    if bit_pos > 0:
        result.append(current_byte)

    return _HuffEncodedResult(result^, total_bits, frequencies^)


def _huffman_decode(
    ref nodes: List[_HuffmanNode],
    root_idx: Int,
    encoded: List[UInt8],
    bit_count: Int,
) -> List[UInt8]:
    var result = List[UInt8]()
    var current_idx = root_idx
    var bits_processed = 0
    var byte_idx = 0

    while bits_processed < bit_count and byte_idx < len(encoded):
        var byte_val = encoded[byte_idx]
        byte_idx += 1

        for bit_pos in range(7, -1, -1):
            if bits_processed >= bit_count:
                break
            var bit = ((Int(byte_val) >> bit_pos) & 1) == 1
            bits_processed += 1

            var node = nodes[current_idx]
            if bit:
                current_idx = node.right
            else:
                current_idx = node.left

            if nodes[current_idx].is_leaf:
                result.append(nodes[current_idx].byte_val)
                current_idx = root_idx

    return result^


struct HuffEncode(Benchmark, Movable):
    var size: Int
    var result: UInt32
    var test_data: List[UInt8]
    var encoded: _HuffEncodedResult

    def __init__(out self, config: Config) raises:
        self.size = config.get_i64("Compress::HuffEncode", "size")
        self.result = 0
        self.test_data = List[UInt8]()
        self.encoded = _HuffEncodedResult()

    def class_name(self) -> String:
        return "Compress::HuffEncode"

    def prepare(mut self, mut helper: Helper) raises:
        self.test_data = generate_test_data(self.size)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var frequencies = List[Int](length=256, fill=0)
        for i in range(len(self.test_data)):
            frequencies[Int(self.test_data[i])] += 1

        var tree_result = _build_huffman_tree_nodes(frequencies)
        ref nodes = tree_result[0]
        var root_idx = tree_result[1]

        var codes = _HuffmanCodes()
        _build_huffman_codes(nodes, root_idx, 0, 0, codes)

        self.encoded = _huffman_encode(self.test_data, codes, frequencies^)
        self.result += UInt32(len(self.encoded.data))

    def checksum(self) -> UInt32:
        return self.result


struct HuffDecode(Benchmark, Movable):
    var size: Int
    var result: UInt32
    var test_data: List[UInt8]
    var decoded: List[UInt8]
    var encoded: _HuffEncodedResult

    def __init__(out self, config: Config) raises:
        self.size = config.get_i64("Compress::HuffDecode", "size")
        self.result = 0
        self.test_data = List[UInt8]()
        self.decoded = List[UInt8]()
        self.encoded = _HuffEncodedResult()

    def class_name(self) -> String:
        return "Compress::HuffDecode"

    def prepare(mut self, mut helper: Helper) raises:
        self.test_data = generate_test_data(self.size)

        var frequencies = List[Int](length=256, fill=0)
        for i in range(len(self.test_data)):
            frequencies[Int(self.test_data[i])] += 1

        var tree_result = _build_huffman_tree_nodes(frequencies)
        ref nodes = tree_result[0]
        var root_idx = tree_result[1]

        var codes = _HuffmanCodes()
        _build_huffman_codes(nodes, root_idx, 0, 0, codes)

        self.encoded = _huffman_encode(self.test_data, codes, frequencies^)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        ref frequencies = self.encoded.frequencies
        ref data = self.encoded.data
        var bit_count = self.encoded.bit_count

        var tree_result = _build_huffman_tree_nodes(frequencies)
        ref nodes = tree_result[0]
        var root_idx = tree_result[1]

        self.decoded = _huffman_decode(nodes, root_idx, data, bit_count)
        self.result += UInt32(len(self.decoded))

    def checksum(self) -> UInt32:
        var r = self.result
        if len(self.decoded) == len(self.test_data):
            var equal = True
            for i in range(len(self.test_data)):
                if self.decoded[i] != self.test_data[i]:
                    equal = False
                    break
            if equal:
                r += 100000
        return r


struct _ArithFreqTable(Copyable, Movable):
    var total: Int
    var low: List[Int]
    var high: List[Int]

    def __init__(out self, frequencies: List[Int]):
        self.total = 0
        for i in range(256):
            self.total += frequencies[i]
        self.low = List[Int](length=256, fill=0)
        self.high = List[Int](length=256, fill=0)

        var cum = 0
        for i in range(256):
            self.low[i] = cum
            cum += frequencies[i]
            self.high[i] = cum


struct _ArithEncodedResult(Copyable, Movable):
    var data: List[UInt8]
    var frequencies: List[Int]

    def __init__(out self):
        self.data = List[UInt8]()
        self.frequencies = List[Int]()

    def __init__(out self, var data: List[UInt8], var frequencies: List[Int]):
        self.data = data^
        self.frequencies = frequencies^


struct _BitOutputStream(Movable):
    var buffer: UInt8
    var bit_pos: Int
    var bytes: List[UInt8]
    var bits_written: Int

    def __init__(out self):
        self.buffer = 0
        self.bit_pos = 0
        self.bytes = List[UInt8]()
        self.bits_written = 0

    def write_bit(mut self, bit: Int):
        self.buffer = (self.buffer << 1) | UInt8(bit & 1)
        self.bit_pos += 1
        self.bits_written += 1

        if self.bit_pos == 8:
            self.bytes.append(self.buffer)
            self.buffer = 0
            self.bit_pos = 0

    def flush(mut self) -> List[UInt8]:
        if self.bit_pos > 0:
            self.buffer <<= UInt8(8 - self.bit_pos)
            self.bytes.append(self.buffer)
        var result = self.bytes^
        self.bytes = List[UInt8]()
        return result^


struct _BitInputStream(Movable):
    var bytes: List[UInt8]
    var byte_pos: Int
    var bit_pos: Int
    var current_byte: UInt8

    def __init__(out self, var bytes: List[UInt8]):
        self.bytes = bytes^
        self.byte_pos = 0
        self.bit_pos = 0
        self.current_byte = 0
        if len(self.bytes) > 0:
            self.current_byte = self.bytes[0]

    def read_bit(mut self) -> Int:
        if self.bit_pos == 8:
            self.byte_pos += 1
            self.bit_pos = 0
            if self.byte_pos < len(self.bytes):
                self.current_byte = self.bytes[self.byte_pos]
            else:
                self.current_byte = 0

        var bit = (Int(self.current_byte) >> (7 - self.bit_pos)) & 1
        self.bit_pos += 1
        return bit


def _arith_encode_data(data: List[UInt8]) -> _ArithEncodedResult:
    var frequencies = List[Int](length=256, fill=0)
    for i in range(len(data)):
        frequencies[Int(data[i])] += 1

    var freq_table = _ArithFreqTable(frequencies)

    var low: UInt64 = 0
    var high: UInt64 = 0xFFFFFFFF
    var pend: Int = 0
    var output = _BitOutputStream()

    for i in range(len(data)):
        var byte = Int(data[i])
        var rng = high - low + 1

        high = (
            low
            + (rng * UInt64(freq_table.high[byte]) // UInt64(freq_table.total))
            - 1
        )
        low = low + (
            rng * UInt64(freq_table.low[byte]) // UInt64(freq_table.total)
        )

        while True:
            if high < 0x80000000:
                output.write_bit(0)
                for _ in range(pend):
                    output.write_bit(1)
                pend = 0
            elif low >= 0x80000000:
                output.write_bit(1)
                for _ in range(pend):
                    output.write_bit(0)
                pend = 0
                low -= 0x80000000
                high -= 0x80000000
            elif low >= 0x40000000 and high < 0xC0000000:
                pend += 1
                low -= 0x40000000
                high -= 0x40000000
            else:
                break

            low <<= 1
            high = (high << 1) | 1
            high &= 0xFFFFFFFF

    pend += 1
    if low < 0x40000000:
        output.write_bit(0)
        for _ in range(pend):
            output.write_bit(1)
    else:
        output.write_bit(1)
        for _ in range(pend):
            output.write_bit(0)

    var result_data = output.flush()
    return _ArithEncodedResult(result_data^, frequencies^)


def _arith_decode_data(encoded: _ArithEncodedResult) -> List[UInt8]:
    ref frequencies = encoded.frequencies
    ref data = encoded.data

    var total = 0
    for i in range(256):
        total += frequencies[i]
    var data_size = total

    var freq_table = _ArithFreqTable(frequencies)

    var result = List[UInt8](length=data_size, fill=0)
    var data_copy = data.copy()
    var input = _BitInputStream(data_copy^)

    var value: UInt64 = 0
    for _ in range(32):
        value = (value << 1) | UInt64(input.read_bit())

    var low: UInt64 = 0
    var high: UInt64 = 0xFFFFFFFF

    for j in range(data_size):
        var rng = high - low + 1
        var scaled = ((value - low + 1) * UInt64(total) - 1) // rng

        var left: Int = 0
        var right: Int = 256
        while left < right:
            var mid = (left + right) // 2
            if UInt64(freq_table.high[mid]) <= scaled:
                left = mid + 1
            else:
                right = mid
        var symbol: UInt8 = UInt8(left)

        result[j] = symbol

        high = (
            low
            + (rng * UInt64(freq_table.high[Int(symbol)]) // UInt64(total))
            - 1
        )
        low = low + (rng * UInt64(freq_table.low[Int(symbol)]) // UInt64(total))

        while True:
            if high < 0x80000000:
                pass
            elif low >= 0x80000000:
                value -= 0x80000000
                low -= 0x80000000
                high -= 0x80000000
            elif low >= 0x40000000 and high < 0xC0000000:
                value -= 0x40000000
                low -= 0x40000000
                high -= 0x40000000
            else:
                break
            low <<= 1
            high = (high << 1) | 1
            value = (value << 1) | UInt64(input.read_bit())

    return result^


struct ArithEncode(Benchmark, Movable):
    var size: Int
    var result: UInt32
    var test_data: List[UInt8]
    var encoded: _ArithEncodedResult

    def __init__(out self, config: Config) raises:
        self.size = config.get_i64("Compress::ArithEncode", "size")
        self.result = 0
        self.test_data = List[UInt8]()
        self.encoded = _ArithEncodedResult()

    def class_name(self) -> String:
        return "Compress::ArithEncode"

    def prepare(mut self, mut helper: Helper) raises:
        self.test_data = generate_test_data(self.size)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.encoded = _arith_encode_data(self.test_data)
        self.result += UInt32(len(self.encoded.data))

    def checksum(self) -> UInt32:
        return self.result


struct ArithDecode(Benchmark, Movable):
    var size: Int
    var result: UInt32
    var test_data: List[UInt8]
    var decoded: List[UInt8]
    var encoded: _ArithEncodedResult

    def __init__(out self, config: Config) raises:
        self.size = config.get_i64("Compress::ArithDecode", "size")
        self.result = 0
        self.test_data = List[UInt8]()
        self.decoded = List[UInt8]()
        self.encoded = _ArithEncodedResult()

    def class_name(self) -> String:
        return "Compress::ArithDecode"

    def prepare(mut self, mut helper: Helper) raises:
        self.test_data = generate_test_data(self.size)

        self.encoded = _arith_encode_data(self.test_data)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.decoded = _arith_decode_data(self.encoded)
        self.result += UInt32(len(self.decoded))

    def checksum(self) -> UInt32:
        var r = self.result
        if len(self.decoded) == len(self.test_data):
            var equal = True
            for i in range(len(self.test_data)):
                if self.decoded[i] != self.test_data[i]:
                    equal = False
                    break
            if equal:
                r += 100000
        return r


struct _LZWResult(Copyable, Movable):
    var data: List[UInt8]
    var dict_size: Int

    def __init__(out self):
        self.data = List[UInt8]()
        self.dict_size = 256

    def __init__(out self, var data: List[UInt8], dict_size: Int):
        self.data = data^
        self.dict_size = dict_size


struct LZWEncode(Benchmark, Movable):
    var size: Int
    var result: UInt32
    var test_data: List[UInt8]
    var encoded: _LZWResult

    def __init__(out self, config: Config) raises:
        self.size = config.get_i64("Compress::LZWEncode", "size")
        self.result = 0
        self.test_data = List[UInt8]()
        self.encoded = _LZWResult()

    def class_name(self) -> String:
        return "Compress::LZWEncode"

    def prepare(mut self, mut helper: Helper) raises:
        self.test_data = generate_test_data(self.size)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.encoded = Self._lzw_encode(self.test_data)
        self.result += UInt32(len(self.encoded.data))

    def checksum(self) -> UInt32:
        return self.result

    @staticmethod
    def _lzw_encode(input: List[UInt8]) raises -> _LZWResult:
        var n = len(input)
        if n == 0:
            return _LZWResult()

        var dict = Dict[String, Int]()
        for i in range(256):
            dict[String(chr(i))] = i

        var next_code = 256
        var result = List[UInt8]()

        var current = String(chr(Int(input[0])))

        for i in range(1, n):
            var next_char = String(chr(Int(input[i])))
            var new_str = current + next_char

            if new_str in dict:
                current = new_str
            else:
                var code = dict[current]
                result.append(UInt8((code >> 8) & 0xFF))
                result.append(UInt8(code & 0xFF))

                dict[new_str] = next_code
                next_code += 1
                current = next_char

        var code = dict[current]
        result.append(UInt8((code >> 8) & 0xFF))
        result.append(UInt8(code & 0xFF))

        return _LZWResult(result^, next_code)


struct LZWDecode(Benchmark, Movable):
    var size: Int
    var result: UInt32
    var test_data: List[UInt8]
    var decoded: List[UInt8]
    var encoded: _LZWResult

    def __init__(out self, config: Config) raises:
        self.size = config.get_i64("Compress::LZWDecode", "size")
        self.result = 0
        self.test_data = List[UInt8]()
        self.decoded = List[UInt8]()
        self.encoded = _LZWResult()

    def class_name(self) -> String:
        return "Compress::LZWDecode"

    def prepare(mut self, mut helper: Helper) raises:
        self.test_data = generate_test_data(self.size)
        self.encoded = LZWEncode._lzw_encode(self.test_data)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.decoded = Self._lzw_decode(self.encoded)
        self.result += UInt32(len(self.decoded))

    def checksum(self) -> UInt32:
        var r = self.result
        if len(self.decoded) == len(self.test_data):
            var equal = True
            for i in range(len(self.test_data)):
                if self.decoded[i] != self.test_data[i]:
                    equal = False
                    break
            if equal:
                r += 100000
        return r

    @staticmethod
    def _lzw_decode(encoded: _LZWResult) -> List[UInt8]:
        ref data = encoded.data
        if len(data) == 0:
            return List[UInt8]()

        var dict = List[String](capacity=4096)
        for i in range(256):
            dict.append(String(chr(i)))

        var result = List[UInt8]()
        var pos = 0

        var high = Int(data[pos])
        var low = Int(data[pos + 1])
        var old_code = (high << 8) | low
        pos += 2

        var old_str = dict[old_code]
        for b in old_str.as_bytes():
            result.append(UInt8(b))

        var next_code = 256

        while pos < len(data):
            high = Int(data[pos])
            low = Int(data[pos + 1])
            var new_code = (high << 8) | low
            pos += 2

            var new_str: String
            if new_code < len(dict):
                new_str = dict[new_code]
            elif new_code == next_code:
                new_str = old_str + String(old_str[byte=0])
            else:
                return List[UInt8]()

            for b in new_str.as_bytes():
                result.append(UInt8(b))

            dict.append(old_str + String(new_str[byte=0]))
            next_code += 1

            old_str = new_str

        return result^


struct CsvPoint(Copyable, Movable):
    var x: Float64
    var y: Float64
    var z: Float64

    def __init__(out self, x: Float64, y: Float64, z: Float64):
        self.x = x
        self.y = y
        self.z = z


struct CsvParse(Benchmark, Movable):
    var rows: Int
    var _checksum: UInt32
    var data: String

    def __init__(out self, config: Config) raises:
        self.rows = config.get_i64("CSV::Parse", "rows")
        self._checksum = 0
        self.data = ""

    def class_name(self) -> String:
        return "CSV::Parse"

    def prepare(mut self, mut helper: Helper) raises:
        self.data = ""
        for i in range(self.rows):
            var c = chr(65 + (i % 26))
            var x = helper.next_float()
            var z = helper.next_float()
            var y = helper.next_float()

            self.data += '"'
            self.data += "point "
            self.data += c
            self.data += '\\n, ""'
            self.data += String(i % 100)
            self.data += '"""'
            self.data += ","
            self.data += Self._format_f64(x)
            self.data += ","
            self.data += ","
            self.data += Self._format_f64(z)
            self.data += ","
            self.data += '"'
            self.data += "["
            self.data += "true" if i % 2 == 0 else "false"
            self.data += "\\n, "
            self.data += String(i % 100)
            self.data += "]"
            self.data += '"'
            self.data += ","
            self.data += Self._format_f64(y)
            self.data += "\n"

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var csv_mod = Python.import_module("csv")
        var io_mod = Python.import_module("io")
        var builtins = Python.import_module("builtins")

        var reader = csv_mod.reader(io_mod.StringIO(self.data))
        var rows_list = builtins.list(reader)

        var x_sum: Float64 = 0.0
        var y_sum: Float64 = 0.0
        var z_sum: Float64 = 0.0
        var count = 0

        for i in range(Int(py=rows_list.__len__())):
            var row = rows_list[i]
            var row_len = Int(py=row.__len__())
            if row_len >= 6:
                var x = atof(StringSlice(String(py=row[1])))
                var z = atof(StringSlice(String(py=row[3])))
                var y = atof(StringSlice(String(py=row[5])))

                x_sum += x
                y_sum += y
                z_sum += z
                count += 1

        if count == 0:
            return

        var x_avg = x_sum / Float64(count)
        var y_avg = y_sum / Float64(count)
        var z_avg = z_sum / Float64(count)

        self._checksum = self._checksum + Helper.checksum_f64(x_avg)
        self._checksum = self._checksum + Helper.checksum_f64(y_avg)
        self._checksum = self._checksum + Helper.checksum_f64(z_avg)

    def checksum(self) -> UInt32:
        return self._checksum

    @staticmethod
    def _format_f64(v: Float64) -> String:
        var result = ""
        var val = v
        if val < 0:
            result += "-"
            val = -val
        var int_part = Int(val)
        result += String(int_part)
        result += "."
        var frac_scaled = Int((val - Float64(int_part)) * 10000000000.0 + 0.5)
        var fs = String(frac_scaled)
        while fs.byte_length() < 10:
            fs = "0" + fs
        result += fs
        return result


struct LogParser(Benchmark, Movable):
    var lines_count: Int
    var log: String
    var _checksum: UInt32
    var compiled_patterns: PythonObject

    def __init__(out self, config: Config) raises:
        self.lines_count = config.get_i64("Etc::LogParser", "lines_count")
        self.log = ""
        self._checksum = 0
        self.compiled_patterns = PythonObject(None)

    def class_name(self) -> String:
        return "Etc::LogParser"

    def prepare(mut self, mut helper: Helper) raises:
        self.log = self._generate_log()

        var re = Python.import_module("re")
        var patterns = Python.list()
        patterns.append(
            Python.tuple("errors", re.compile(r" [5][0-9]{2} | [4][0-9]{2} "))
        )
        patterns.append(
            Python.tuple(
                "bots",
                re.compile(
                    r"bot|crawler|scanner|spider|indexing|crawl|robot|spider",
                    re.IGNORECASE,
                ),
            )
        )
        patterns.append(
            Python.tuple(
                "suspicious",
                re.compile(r"etc/passwd|wp-admin|\.\./", re.IGNORECASE),
            )
        )
        patterns.append(Python.tuple("ips", re.compile(r"\d+\.\d+\.\d+\.35")))
        patterns.append(Python.tuple("api_calls", re.compile(r'/api/[^ " ]+')))
        patterns.append(
            Python.tuple("post_requests", re.compile(r"POST [^ ]* HTTP"))
        )
        patterns.append(
            Python.tuple(
                "auth_attempts", re.compile(r"/login|/signin", re.IGNORECASE)
            )
        )
        patterns.append(
            Python.tuple("methods", re.compile(r"get|post|put", re.IGNORECASE))
        )
        patterns.append(
            Python.tuple(
                "emails",
                re.compile(r"[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}"),
            )
        )
        patterns.append(
            Python.tuple("passwords", re.compile(r'password=[^&\s"]+'))
        )
        patterns.append(
            Python.tuple(
                "tokens", re.compile(r'token=[^&\s"]+|api[_-]?key=[^&\s"]+')
            )
        )
        patterns.append(
            Python.tuple("sessions", re.compile(r'session[_-]?id=[^&\s"]+'))
        )
        patterns.append(
            Python.tuple(
                "peak_hours",
                re.compile(r"\[\d+/\w+/\d+:1[3-7]:\d+:\d+ [+\-]\d+\]"),
            )
        )

        self.compiled_patterns = patterns

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var patterns = self.compiled_patterns
        for i in range(Int(py=patterns.__len__())):
            var pair = patterns[i]
            var name = pair[0]
            var compiled = pair[1]

            var count = 0
            var iter = compiled.finditer(self.log)
            for _ in iter:
                count += 1
            self._checksum += UInt32(count)

    def checksum(self) -> UInt32:
        return self._checksum

    def _generate_log(self) -> String:
        var ips = List[String]()
        for i in range(1, 256):
            ips.append(String("192.168.1.", i))

        var methods = List[String]()
        methods.append("GET")
        methods.append("POST")
        methods.append("PUT")
        methods.append("DELETE")

        var paths = List[String]()
        paths.append("/index.html")
        paths.append("/api/users")
        paths.append("/admin")
        paths.append("/images/logo.png")
        paths.append("/etc/passwd")
        paths.append("/wp-admin/setup.php")

        var statuses = List[Int]()
        statuses.append(200)
        statuses.append(201)
        statuses.append(301)
        statuses.append(302)
        statuses.append(400)
        statuses.append(401)
        statuses.append(403)
        statuses.append(404)
        statuses.append(500)
        statuses.append(502)
        statuses.append(503)

        var agents = List[String]()
        agents.append("Mozilla/5.0")
        agents.append("Googlebot/2.1")
        agents.append("curl/7.68.0")
        agents.append("scanner/2.0")

        var users = List[String]()
        users.append("john")
        users.append("jane")
        users.append("alex")
        users.append("sarah")
        users.append("mike")
        users.append("anna")
        users.append("david")
        users.append("elena")

        var domains = List[String]()
        domains.append("example.com")
        domains.append("gmail.com")
        domains.append("yahoo.com")
        domains.append("hotmail.com")
        domains.append("company.org")
        domains.append("mail.ru")

        var log = ""
        for i in range(self.lines_count):
            log += ips[i % len(ips)]
            log += " - - ["
            log += String(i % 31)
            log += "/Oct/2023:"
            log += String(i % 60)
            log += ':55:36 +0000] "'
            log += methods[i % len(methods)]
            log += " "

            if i % 3 == 0:
                log += "/login?email="
                log += users[i % len(users)]
                log += String(i % 100)
                log += "@"
                log += domains[i % len(domains)]
                log += "&password=secret"
                log += String(i % 10000)
            elif i % 5 == 0:
                log += "/api/data?token="
                var token = "abcdef123456"
                for _ in range((i % 3) + 1):
                    log += token
            elif i % 7 == 0:
                log += "/user/profile?session_id=sess_"
                log += Self._to_hex(i * 12345)
            else:
                log += paths[i % len(paths)]

            log += ' HTTP/1.1" '
            log += String(statuses[i % len(statuses)])
            log += ' 2326 "http://'
            log += domains[i % len(domains)]
            log += '" "'
            log += agents[i % len(agents)]
            log += '"\n'

        return log

    @staticmethod
    def _to_hex(value: Int) -> String:
        if value == 0:
            return "0"

        var hex_chars = "0123456789abcdef"
        var result = ""
        var v = value

        while v > 0:
            var digit = v & 15
            result = String(hex_chars[byte=digit]) + result
            v >>= 4

        return result


struct TemplateRegex(Benchmark, Movable):
    var count: Int
    var text: String
    var rendered: String
    var checksum_val: UInt32
    var vars: Dict[String, String]
    var compiled_pattern: PythonObject

    def __init__(out self, config: Config) raises:
        self.count = config.get_i64("Template::Regex", "count")
        self.text = ""
        self.rendered = ""
        self.checksum_val = 0
        self.vars = Dict[String, String]()
        var re = Python.import_module("re")
        self.compiled_pattern = re.compile(r"\{\{(.*?)\}\}")

    def class_name(self) -> String:
        return "Template::Regex"

    def prepare(mut self, mut helper: Helper) raises:
        self.vars = Dict[String, String]()
        Self._prepare_template(self.count, self.vars)
        self.text = Self._build_text(self.count, self.vars)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var result = ""
        var text_len = self.text.byte_length()
        var last_pos = 0

        for _match in self.compiled_pattern.finditer(self.text):
            var match_start = Int(py=_match.start())
            var match_end = Int(py=_match.end())
            var key = String(py=_match.group(1))

            for k in range(last_pos, match_start):
                result += String(self.text[byte=k])

            var stripped = String(key.strip())

            if stripped in self.vars:
                result += self.vars[stripped]

            last_pos = match_end

        for k in range(last_pos, text_len):
            result += String(self.text[byte=k])

        self.rendered = result
        self.checksum_val += UInt32(self.rendered.byte_length())

    def checksum(self) -> UInt32:
        return (
            self.checksum_val + Helper.checksum_string(self.rendered)
        ) & 0xFFFFFFFF

    @staticmethod
    def _prepare_template(count: Int, mut vars: Dict[String, String]):
        var first_names = List[String]()
        first_names.append("John")
        first_names.append("Jane")
        first_names.append("Bob")
        first_names.append("Alice")
        first_names.append("Charlie")
        first_names.append("Diana")
        first_names.append("Sarah")
        first_names.append("Mike")

        var last_names = List[String]()
        last_names.append("Smith")
        last_names.append("Johnson")
        last_names.append("Brown")
        last_names.append("Taylor")
        last_names.append("Wilson")
        last_names.append("Davis")
        last_names.append("Miller")
        last_names.append("Jones")

        var cities = List[String]()
        cities.append("New York")
        cities.append("Los Angeles")
        cities.append("Chicago")
        cities.append("Houston")
        cities.append("Phoenix")
        cities.append("San Francisco")

        vars["TITLE"] = "Template title"

        for i in range(count):
            vars["FIRST_NAME" + String(i)] = first_names[i % len(first_names)]
            vars["LAST_NAME" + String(i)] = last_names[i % len(last_names)]
            vars["CITY" + String(i)] = cities[i % len(cities)]

    @staticmethod
    def _build_text(count: Int, vars: Dict[String, String]) -> String:
        var lorem = (
            "Lorem {ipsum} dolor {sit} amet, consectetur adipiscing elit. Sed"
            " do eiusmod tempor incididunt ut labore {et} dolore magna aliqua. "
        )

        var text = "<html><body>"
        text += "<h1>{{TITLE}}</h1>"
        text += "<p>"
        text += lorem
        text += "</p>"
        text += "<table>"

        for i in range(count):
            if i % 3 == 0:
                text += "<!-- {comment} -->"
            text += "<tr>"
            text += "<td>{{ FIRST_NAME" + String(i) + " }}</td>"
            text += "<td>{{LAST_NAME" + String(i) + "}}</td>"
            text += "<td>{{  CITY" + String(i) + "  }}</td>"
            text += "<td>{balance: " + String(i % 100) + "}</td>"
            text += "</tr>\n"

        text += "</table>"
        text += "</body></html>"
        return text


struct TemplateParse(Benchmark, Movable):
    var count: Int
    var text: String
    var rendered: String
    var checksum_val: UInt32
    var vars: Dict[String, String]

    def __init__(out self, config: Config) raises:
        self.count = config.get_i64("Template::Parse", "count")
        self.text = ""
        self.rendered = ""
        self.checksum_val = 0
        self.vars = Dict[String, String]()

    def class_name(self) -> String:
        return "Template::Parse"

    def prepare(mut self, mut helper: Helper) raises:
        self.vars = Dict[String, String]()
        TemplateRegex._prepare_template(self.count, self.vars)
        self.text = TemplateRegex._build_text(self.count, self.vars)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var text = self.text
        var text_len = text.byte_length()
        var text_bytes = text.as_bytes()

        var builder = String(capacity_bytes=text_len * 2)

        var i = 0
        var chunk_start = 0

        while i < text_len:
            if (
                i + 1 < text_len
                and text_bytes[i] == 123
                and text_bytes[i + 1] == 123
            ):
                if chunk_start < i:
                    builder._iadd(text_bytes[chunk_start:i])

                var key_start = i + 2
                var j = key_start
                while j + 1 < text_len:
                    if text_bytes[j] == 125 and text_bytes[j + 1] == 125:
                        break
                    j += 1

                if j + 1 < text_len:
                    var key = String(
                        StringSpan(
                            unsafe_from_utf8=text_bytes[key_start:j]
                        ).strip()
                    )

                    var val = self.vars.get(key)
                    if val:
                        builder._iadd(val[].as_bytes())

                    i = j + 2
                    chunk_start = i
                    continue

            i += 1

        if chunk_start < text_len:
            builder._iadd(text_bytes[chunk_start:text_len])

        self.rendered = builder
        self.checksum_val += UInt32(self.rendered.byte_length())

    def checksum(self) -> UInt32:
        return (
            self.checksum_val + Helper.checksum_string(self.rendered)
        ) & 0xFFFFFFFF


def generate_json_data(n: Int, mut helper: Helper) raises -> String:
    var json_mod = Python.import_module("json")
    var coords = Python.list()

    for _ in range(n):
        var coord = Python.dict()
        coord["x"] = round(helper.next_float(), 8)
        coord["y"] = round(helper.next_float(), 8)
        coord["z"] = round(helper.next_float(), 8)
        coord["name"] = String("%.7f %d").format(
            helper.next_float(), helper.next_int(10000)
        )
        coord["opts"] = Python.dict()
        coord["opts"]["1"] = Python.tuple(1, True)
        coords.append(coord)

    var root = Python.dict()
    root["coordinates"] = coords
    root["info"] = "some info"

    return String(py=json_mod.dumps(root))


struct JsonGenerate(Benchmark, Movable):
    var n: Int
    var result: UInt32
    var text: String

    def __init__(out self, config: Config) raises:
        self.n = config.get_i64("Json::Generate", "coords")
        self.result = 0
        self.text = ""

    def class_name(self) -> String:
        return "Json::Generate"

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        self.text = generate_json_data(self.n, helper)

        var prefix = '{"coordinates":'
        if self.text.startswith(prefix):
            self.result += 1

    def checksum(self) -> UInt32:
        return self.result


struct JsonParseDom(Benchmark, Movable):
    var n: Int
    var text: String
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.n = config.get_i64("Json::ParseDom", "coords")
        self.text = ""
        self.result = 0

    def class_name(self) -> String:
        return "Json::ParseDom"

    def prepare(mut self, mut helper: Helper) raises:
        self.text = generate_json_data(self.n, helper)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var json_mod = Python.import_module("json")
        var data = json_mod.loads(self.text)

        var coordinates = data["coordinates"]
        var len_coords = Float64(Int(py=coordinates.__len__()))

        var x_sum: Float64 = 0.0
        var y_sum: Float64 = 0.0
        var z_sum: Float64 = 0.0

        for i in range(Int(py=coordinates.__len__())):
            var coord = coordinates[i]
            x_sum += Float64(py=coord["x"])
            y_sum += Float64(py=coord["y"])
            z_sum += Float64(py=coord["z"])

        self.result += Helper.checksum_f64(x_sum / len_coords)
        self.result += Helper.checksum_f64(y_sum / len_coords)
        self.result += Helper.checksum_f64(z_sum / len_coords)

    def checksum(self) -> UInt32:
        return self.result


struct JsonParseMapping(Benchmark, Movable):
    var n: Int
    var text: String
    var result: UInt32

    def __init__(out self, config: Config) raises:
        self.n = config.get_i64("Json::ParseMapping", "coords")
        self.text = ""
        self.result = 0

    def class_name(self) -> String:
        return "Json::ParseMapping"

    def prepare(mut self, mut helper: Helper) raises:
        self.text = generate_json_data(self.n, helper)

    def run(mut self, iteration_id: Int, mut helper: Helper) raises:
        var json_mod = Python.import_module("json")
        var data = json_mod.loads(self.text)

        var coordinates = data["coordinates"]
        var len_coords = Float64(Int(py=coordinates.__len__()))

        var x_sum: Float64 = 0.0
        var y_sum: Float64 = 0.0
        var z_sum: Float64 = 0.0

        for i in range(Int(py=coordinates.__len__())):
            var coord = coordinates[i]
            x_sum += Float64(py=coord["x"])
            y_sum += Float64(py=coord["y"])
            z_sum += Float64(py=coord["z"])

        self.result += Helper.checksum_f64(x_sum / len_coords)
        self.result += Helper.checksum_f64(y_sum / len_coords)
        self.result += Helper.checksum_f64(z_sum / len_coords)

    def checksum(self) -> UInt32:
        return self.result


comptime BenchVariant = Variant[
    BinarytreesObj,
    BinarytreesArena,
    BrainfuckArray,
    BrainfuckRecursion,
    MatmulSingle,
    MatmulT4,
    MatmulT8,
    MatmulT16,
    Base64Encode,
    Base64Decode,
    Fannkuchredux,
    Spectralnorm,
    Mandelbrot,
    Nbody,
    DistanceJaro,
    DistanceNGram,
    MazeGenerator,
    MazeBFS,
    MazeAStar,
    HashSHA256,
    HashCRC32,
    GraphBFS,
    GraphDFS,
    GraphAStar,
    SortQuick,
    SortMerge,
    SortSelf,
    Sieve,
    TextRaytracer,
    NeuralNet,
    CacheSimulation,
    GameOfLife,
    Words,
    CalculatorAst,
    CalculatorInterpreter,
    BWTEncode,
    BWTDecode,
    HuffEncode,
    HuffDecode,
    ArithEncode,
    ArithDecode,
    LZWEncode,
    LZWDecode,
    CsvParse,
    LogParser,
    TemplateRegex,
    TemplateParse,
    JsonGenerate,
    JsonParseDom,
    JsonParseMapping,
]


def create_benchmark(name: String, config: Config) raises -> BenchVariant:
    if name == "Binarytrees::Obj":
        return BenchVariant(BinarytreesObj(config))
    elif name == "Binarytrees::Arena":
        return BenchVariant(BinarytreesArena(config))
    elif name == "Brainfuck::Array":
        return BenchVariant(BrainfuckArray(config))
    elif name == "Brainfuck::Recursion":
        return BenchVariant(BrainfuckRecursion(config))
    elif name == "Matmul::Single":
        return BenchVariant(MatmulSingle(config))
    elif name == "Matmul::T4":
        return BenchVariant(MatmulT4(config))
    elif name == "Matmul::T8":
        return BenchVariant(MatmulT8(config))
    elif name == "Matmul::T16":
        return BenchVariant(MatmulT16(config))
    elif name == "Base64::Encode":
        return BenchVariant(Base64Encode(config))
    elif name == "Base64::Decode":
        return BenchVariant(Base64Decode(config))
    elif name == "CLBG::Fannkuchredux":
        return BenchVariant(Fannkuchredux(config))
    elif name == "CLBG::Spectralnorm":
        return BenchVariant(Spectralnorm(config))
    elif name == "CLBG::Mandelbrot":
        return BenchVariant(Mandelbrot(config))
    elif name == "CLBG::Nbody":
        return BenchVariant(Nbody(config))
    elif name == "Distance::Jaro":
        return BenchVariant(DistanceJaro(config))
    elif name == "Distance::NGram":
        return BenchVariant(DistanceNGram(config))
    elif name == "Maze::Generator":
        return BenchVariant(MazeGenerator(config))
    elif name == "Maze::BFS":
        return BenchVariant(MazeBFS(config))
    elif name == "Maze::AStar":
        return BenchVariant(MazeAStar(config))
    elif name == "Hash::SHA256":
        return BenchVariant(HashSHA256(config))
    elif name == "Hash::CRC32":
        return BenchVariant(HashCRC32(config))
    elif name == "Graph::BFS":
        return BenchVariant(GraphBFS(config))
    elif name == "Graph::DFS":
        return BenchVariant(GraphDFS(config))
    elif name == "Graph::AStar":
        return BenchVariant(GraphAStar(config))
    elif name == "Sort::Quick":
        return BenchVariant(SortQuick(config))
    elif name == "Sort::Merge":
        return BenchVariant(SortMerge(config))
    elif name == "Sort::Self":
        return BenchVariant(SortSelf(config))
    elif name == "Etc::Sieve":
        return BenchVariant(Sieve(config))
    elif name == "Etc::TextRaytracer":
        return BenchVariant(TextRaytracer(config))
    elif name == "Etc::NeuralNet":
        return BenchVariant(NeuralNet(config))
    elif name == "Etc::CacheSimulation":
        return BenchVariant(CacheSimulation(config))
    elif name == "Etc::GameOfLife":
        return BenchVariant(GameOfLife(config))
    elif name == "Etc::Words":
        return BenchVariant(Words(config))
    elif name == "Calculator::Ast":
        return BenchVariant(CalculatorAst(config))
    elif name == "Calculator::Interpreter":
        return BenchVariant(CalculatorInterpreter(config))
    elif name == "Compress::BWTEncode":
        return BenchVariant(BWTEncode(config))
    elif name == "Compress::BWTDecode":
        return BenchVariant(BWTDecode(config))
    elif name == "Compress::HuffEncode":
        return BenchVariant(HuffEncode(config))
    elif name == "Compress::HuffDecode":
        return BenchVariant(HuffDecode(config))
    elif name == "Compress::ArithEncode":
        return BenchVariant(ArithEncode(config))
    elif name == "Compress::ArithDecode":
        return BenchVariant(ArithDecode(config))
    elif name == "Compress::LZWEncode":
        return BenchVariant(LZWEncode(config))
    elif name == "Compress::LZWDecode":
        return BenchVariant(LZWDecode(config))
    elif name == "CSV::Parse":
        return BenchVariant(CsvParse(config))
    elif name == "Etc::LogParser":
        return BenchVariant(LogParser(config))
    elif name == "Template::Regex":
        return BenchVariant(TemplateRegex(config))
    elif name == "Template::Parse":
        return BenchVariant(TemplateParse(config))
    elif name == "Json::Generate":
        return BenchVariant(JsonGenerate(config))
    elif name == "Json::ParseDom":
        return BenchVariant(JsonParseDom(config))
    elif name == "Json::ParseMapping":
        return BenchVariant(JsonParseMapping(config))
    else:
        raise Error(String("Unknown benchmark: ", name))


def dispatch_bench(
    mut bench: BenchVariant,
    name: String,
    config: Config,
    mut helper: Helper,
    mut summary_time: Float64,
    mut ok: Int,
    mut fails: Int,
) raises:
    if bench.isa[BinarytreesObj]():
        run_single(
            name, bench[BinarytreesObj], config, helper, summary_time, ok, fails
        )
    elif bench.isa[BinarytreesArena]():
        run_single(
            name,
            bench[BinarytreesArena],
            config,
            helper,
            summary_time,
            ok,
            fails,
        )
    elif bench.isa[BrainfuckArray]():
        run_single(
            name, bench[BrainfuckArray], config, helper, summary_time, ok, fails
        )
    elif bench.isa[BrainfuckRecursion]():
        run_single(
            name,
            bench[BrainfuckRecursion],
            config,
            helper,
            summary_time,
            ok,
            fails,
        )
    elif bench.isa[MatmulSingle]():
        run_single(
            name, bench[MatmulSingle], config, helper, summary_time, ok, fails
        )
    elif bench.isa[MatmulT4]():
        run_single(
            name, bench[MatmulT4], config, helper, summary_time, ok, fails
        )
    elif bench.isa[MatmulT8]():
        run_single(
            name, bench[MatmulT8], config, helper, summary_time, ok, fails
        )
    elif bench.isa[MatmulT16]():
        run_single(
            name, bench[MatmulT16], config, helper, summary_time, ok, fails
        )
    elif bench.isa[Base64Encode]():
        run_single(
            name, bench[Base64Encode], config, helper, summary_time, ok, fails
        )
    elif bench.isa[Base64Decode]():
        run_single(
            name, bench[Base64Decode], config, helper, summary_time, ok, fails
        )
    elif bench.isa[Fannkuchredux]():
        run_single(
            name, bench[Fannkuchredux], config, helper, summary_time, ok, fails
        )
    elif bench.isa[Spectralnorm]():
        run_single(
            name, bench[Spectralnorm], config, helper, summary_time, ok, fails
        )
    elif bench.isa[Mandelbrot]():
        run_single(
            name, bench[Mandelbrot], config, helper, summary_time, ok, fails
        )
    elif bench.isa[Nbody]():
        run_single(name, bench[Nbody], config, helper, summary_time, ok, fails)
    elif bench.isa[DistanceJaro]():
        run_single(
            name, bench[DistanceJaro], config, helper, summary_time, ok, fails
        )
    elif bench.isa[DistanceNGram]():
        run_single(
            name, bench[DistanceNGram], config, helper, summary_time, ok, fails
        )
    elif bench.isa[MazeGenerator]():
        run_single(
            name, bench[MazeGenerator], config, helper, summary_time, ok, fails
        )
    elif bench.isa[MazeBFS]():
        run_single(
            name, bench[MazeBFS], config, helper, summary_time, ok, fails
        )
    elif bench.isa[MazeAStar]():
        run_single(
            name, bench[MazeAStar], config, helper, summary_time, ok, fails
        )
    elif bench.isa[HashSHA256]():
        run_single(
            name, bench[HashSHA256], config, helper, summary_time, ok, fails
        )
    elif bench.isa[HashCRC32]():
        run_single(
            name, bench[HashCRC32], config, helper, summary_time, ok, fails
        )
    elif bench.isa[GraphBFS]():
        run_single(
            name, bench[GraphBFS], config, helper, summary_time, ok, fails
        )
    elif bench.isa[GraphDFS]():
        run_single(
            name, bench[GraphDFS], config, helper, summary_time, ok, fails
        )
    elif bench.isa[GraphAStar]():
        run_single(
            name, bench[GraphAStar], config, helper, summary_time, ok, fails
        )
    elif bench.isa[SortQuick]():
        run_single(
            name, bench[SortQuick], config, helper, summary_time, ok, fails
        )
    elif bench.isa[SortMerge]():
        run_single(
            name, bench[SortMerge], config, helper, summary_time, ok, fails
        )
    elif bench.isa[SortSelf]():
        run_single(
            name, bench[SortSelf], config, helper, summary_time, ok, fails
        )
    elif bench.isa[Sieve]():
        run_single(name, bench[Sieve], config, helper, summary_time, ok, fails)
    elif bench.isa[TextRaytracer]():
        run_single(
            name, bench[TextRaytracer], config, helper, summary_time, ok, fails
        )
    elif bench.isa[NeuralNet]():
        run_single(
            name, bench[NeuralNet], config, helper, summary_time, ok, fails
        )
    elif bench.isa[CacheSimulation]():
        run_single(
            name,
            bench[CacheSimulation],
            config,
            helper,
            summary_time,
            ok,
            fails,
        )
    elif bench.isa[GameOfLife]():
        run_single(
            name, bench[GameOfLife], config, helper, summary_time, ok, fails
        )
    elif bench.isa[Words]():
        run_single(name, bench[Words], config, helper, summary_time, ok, fails)
    elif bench.isa[CalculatorAst]():
        run_single(
            name, bench[CalculatorAst], config, helper, summary_time, ok, fails
        )
    elif bench.isa[CalculatorInterpreter]():
        run_single(
            name,
            bench[CalculatorInterpreter],
            config,
            helper,
            summary_time,
            ok,
            fails,
        )
    elif bench.isa[BWTEncode]():
        run_single(
            name, bench[BWTEncode], config, helper, summary_time, ok, fails
        )
    elif bench.isa[BWTDecode]():
        run_single(
            name, bench[BWTDecode], config, helper, summary_time, ok, fails
        )
    elif bench.isa[HuffEncode]():
        run_single(
            name, bench[HuffEncode], config, helper, summary_time, ok, fails
        )
    elif bench.isa[HuffDecode]():
        run_single(
            name, bench[HuffDecode], config, helper, summary_time, ok, fails
        )
    elif bench.isa[ArithEncode]():
        run_single(
            name, bench[ArithEncode], config, helper, summary_time, ok, fails
        )
    elif bench.isa[ArithDecode]():
        run_single(
            name, bench[ArithDecode], config, helper, summary_time, ok, fails
        )
    elif bench.isa[LZWEncode]():
        run_single(
            name, bench[LZWEncode], config, helper, summary_time, ok, fails
        )
    elif bench.isa[LZWDecode]():
        run_single(
            name, bench[LZWDecode], config, helper, summary_time, ok, fails
        )
    elif bench.isa[CsvParse]():
        run_single(
            name, bench[CsvParse], config, helper, summary_time, ok, fails
        )
    elif bench.isa[LogParser]():
        run_single(
            name, bench[LogParser], config, helper, summary_time, ok, fails
        )
    elif bench.isa[TemplateRegex]():
        run_single(
            name, bench[TemplateRegex], config, helper, summary_time, ok, fails
        )
    elif bench.isa[TemplateParse]():
        run_single(
            name, bench[TemplateParse], config, helper, summary_time, ok, fails
        )
    elif bench.isa[JsonGenerate]():
        run_single(
            name, bench[JsonGenerate], config, helper, summary_time, ok, fails
        )
    elif bench.isa[JsonParseDom]():
        run_single(
            name, bench[JsonParseDom], config, helper, summary_time, ok, fails
        )
    elif bench.isa[JsonParseMapping]():
        run_single(
            name,
            bench[JsonParseMapping],
            config,
            helper,
            summary_time,
            ok,
            fails,
        )


def run_benchmarks(config: Config, single_bench: Optional[String]) raises:
    var summary_time: Float64 = 0.0
    var ok: Int = 0
    var fails: Int = 0
    var helper = Helper()

    for i in range(len(config.entries)):
        ref entry = config.entries[i]
        var bench_name = entry.name

        if single_bench:
            var target = single_bench[]
            if target.lower() not in bench_name.lower():
                continue

        var bench = create_benchmark(bench_name, config)
        dispatch_bench(
            bench, bench_name, config, helper, summary_time, ok, fails
        )

    print(
        String(
            "Summary: ",
            summary_time,
            "s, ",
            ok + fails,
            ", ",
            ok,
            ", ",
            fails,
        )
    )
    if fails > 0:
        raise Error("Benchmarks failed")


def run_single[
    BenchType: Benchmark
](
    name: String,
    mut bench: BenchType,
    config: Config,
    mut helper: Helper,
    mut summary_time: Float64,
    mut ok: Int,
    mut fails: Int,
) raises:
    helper.reset()
    bench.prepare(helper)
    var warmup_iters = bench.warmup_iterations(config)
    bench.warmup(warmup_iters, helper)
    helper.reset()

    var iters = bench.iterations(config)
    var t = perf_counter_ns()
    for i in range(iters):
        bench.run(i, helper)
    var time_delta = Float64(perf_counter_ns() - t) / 1_000_000_000.0

    var check = bench.checksum()
    var expect = bench.expected_checksum(config)

    var status: String
    if check == expect:
        status = "OK"
        ok += 1
    else:
        status = String("ERR[actual=", check, ", expected=", expect, "]")
        fails += 1

    print(String(name, ": ", status, " in ", time_delta, "s"))
    summary_time += time_delta


def main() raises:
    var args = argv()
    var argc = len(args)

    var config_path: String
    var single_bench: Optional[String] = Optional[String](None)

    if argc > 1:
        config_path = String(args[1])
    else:
        config_path = "./test.js"

    if argc > 2:
        single_bench = Optional[String](String(args[2]))

    with open("/tmp/recompile_marker", "w") as f:
        f.write("RECOMPILE_MARKER_0")
    var config = Config(config_path)
    run_benchmarks(config, single_bench)
