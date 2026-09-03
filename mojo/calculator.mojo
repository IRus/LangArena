from helper import Helper
from benchmark import Benchmark, Config


comptime CALC_NUMBER = 0
comptime CALC_VARIABLE = 1
comptime CALC_BINARY = 2
comptime CALC_ASSIGN = 3

comptime CHAR_0 = Byte(ord("0"))
comptime CHAR_9 = Byte(ord("9"))
comptime CHAR_A = Byte(ord("A"))
comptime CHAR_Z = Byte(ord("Z"))
comptime CHAR_a = Byte(ord("a"))
comptime CHAR_z = Byte(ord("z"))
comptime CHAR_PLUS = Byte(ord("+"))
comptime CHAR_MINUS = Byte(ord("-"))
comptime CHAR_STAR = Byte(ord("*"))
comptime CHAR_SLASH = Byte(ord("/"))
comptime CHAR_PERCENT = Byte(ord("%"))
comptime CHAR_LPAREN = Byte(ord("("))
comptime CHAR_RPAREN = Byte(ord(")"))
comptime CHAR_EQUAL = Byte(ord("="))
comptime CHAR_SPACE = Byte(ord(" "))
comptime CHAR_TAB = Byte(ord("\t"))
comptime CHAR_NEWLINE = Byte(ord("\n"))
comptime CHAR_RETURN = Byte(ord("\r"))
comptime CHAR_SEMICOLON = Byte(ord(";"))


struct _CalcNode(Copyable, ImplicitlyCopyable):
    var kind: Int
    var value: Int
    var name: String
    var op: Byte
    var left: Int
    var right: Int

    def __init__(out self, kind: Int):
        self.kind = kind
        self.value = 0
        self.name = ""
        self.op = 0
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
                self._byte_at(self.pos) == CHAR_NEWLINE
                or self._byte_at(self.pos) == CHAR_SEMICOLON
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

            var ch = self._byte_at(self.pos)
            if ch == CHAR_PLUS or ch == CHAR_MINUS:
                self.pos += 1
                var right_idx = self._parse_term()
                var new_node = _CalcNode(CALC_BINARY)
                new_node.op = ch
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

            var ch = self._byte_at(self.pos)
            if ch == CHAR_STAR or ch == CHAR_SLASH or ch == CHAR_PERCENT:
                self.pos += 1
                var right_idx = self._parse_factor()
                var new_node = _CalcNode(CALC_BINARY)
                new_node.op = ch
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

        var ch = self._byte_at(self.pos)

        if ch >= CHAR_0 and ch <= CHAR_9:
            return self._parse_number()
        elif (ch >= CHAR_a and ch <= CHAR_z) or (ch >= CHAR_A and ch <= CHAR_Z):
            return self._parse_variable()
        elif ch == CHAR_LPAREN:
            self.pos += 1
            var node_idx = self._parse_expression()
            self._skip_whitespace()
            if (
                self.pos < self.length
                and self._byte_at(self.pos) == CHAR_RPAREN
            ):
                self.pos += 1
            return node_idx

        return self._add_number(0)

    def _parse_number(mut self) -> Int:
        var v: Int = 0
        while self.pos < self.length:
            var ch = self._byte_at(self.pos)
            if ch >= CHAR_0 and ch <= CHAR_9:
                v = v * 10 + Int(ch - CHAR_0)
                self.pos += 1
            else:
                break
        return self._add_number(v)

    def _parse_variable(mut self) -> Int:
        var start = self.pos
        while self.pos < self.length:
            var ch = self._byte_at(self.pos)
            if (
                (ch >= CHAR_a and ch <= CHAR_z)
                or (ch >= CHAR_A and ch <= CHAR_Z)
                or (ch >= CHAR_0 and ch <= CHAR_9)
            ):
                self.pos += 1
            else:
                break

        var var_name = String(self.input[byte = start : self.pos])

        self._skip_whitespace()
        if self.pos < self.length and self._byte_at(self.pos) == CHAR_EQUAL:
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
            var ch = self._byte_at(self.pos)
            if (
                ch == CHAR_SPACE
                or ch == CHAR_TAB
                or ch == CHAR_NEWLINE
                or ch == CHAR_RETURN
            ):
                self.pos += 1
            else:
                break

    @always_inline
    def _byte_at(self, pos: Int) -> Byte:
        return self.input.as_bytes()[pos]


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
            ref last_node = self.parser.nodes[last_idx]
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
            if node.op == CHAR_PLUS:
                return left + right
            elif node.op == CHAR_MINUS:
                return left - right
            elif node.op == CHAR_STAR:
                return left * right
            elif node.op == CHAR_SLASH:
                return Self._simple_div(left, right)
            elif node.op == CHAR_PERCENT:
                return Self._simple_mod(left, right)
            return 0
        elif node.kind == CALC_ASSIGN:
            var value = self._evaluate(node.left, variables)
            variables[node.name] = value
            return value

        return 0
