from helper import Helper
from benchmark import Benchmark, Config


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
