package LangArena

type MazeCellKind int

const (
	Wall MazeCellKind = iota
	Space
	Start
	Finish
	Border
	Path
)

func (k MazeCellKind) IsWalkable() bool {
	return k == Space || k == Start || k == Finish
}

type MazeCell struct {
	Kind      MazeCellKind
	Neighbors []*MazeCell
	X, Y      int
}

func NewMazeCell(x, y int) *MazeCell {
	return &MazeCell{
		Kind:      Wall,
		Neighbors: make([]*MazeCell, 0, 4),
		X:         x,
		Y:         y,
	}
}

func (c *MazeCell) AddNeighbor(cell *MazeCell) {
	c.Neighbors = append(c.Neighbors, cell)
}

func (c *MazeCell) Reset() {
	if c.Kind == Space {
		c.Kind = Wall
	}
}

type Maze struct {
	Width  int
	Height int
	Cells  [][]*MazeCell
	Start  *MazeCell
	Finish *MazeCell
}

func NewMaze(width, height int) *Maze {
	if width < 5 {
		width = 5
	}
	if height < 5 {
		height = 5
	}

	cells := make([][]*MazeCell, height)
	for y := 0; y < height; y++ {
		cells[y] = make([]*MazeCell, width)
		for x := 0; x < width; x++ {
			cells[y][x] = NewMazeCell(x, y)
		}
	}

	maze := &Maze{
		Width:  width,
		Height: height,
		Cells:  cells,
	}

	maze.Start = cells[1][1]
	maze.Finish = cells[height-2][width-2]
	maze.Start.Kind = Start
	maze.Finish.Kind = Finish

	return maze
}

func (m *Maze) UpdateNeighbors() {

	for y := 0; y < m.Height; y++ {
		for x := 0; x < m.Width; x++ {
			m.Cells[y][x].Neighbors = m.Cells[y][x].Neighbors[:0]
		}
	}

	for y := 0; y < m.Height; y++ {
		for x := 0; x < m.Width; x++ {
			cell := m.Cells[y][x]

			if x > 0 && y > 0 && x < m.Width-1 && y < m.Height-1 {
				cell.AddNeighbor(m.Cells[y-1][x])
				cell.AddNeighbor(m.Cells[y+1][x])
				cell.AddNeighbor(m.Cells[y][x+1])
				cell.AddNeighbor(m.Cells[y][x-1])

				for t := 0; t < 4; t++ {
					i := NextInt(4)
					j := NextInt(4)
					if i != j && i < len(cell.Neighbors) && j < len(cell.Neighbors) {
						cell.Neighbors[i], cell.Neighbors[j] = cell.Neighbors[j], cell.Neighbors[i]
					}
				}
			} else {
				cell.Kind = Border
			}
		}
	}
}

func (m *Maze) Reset() {
	for y := 0; y < m.Height; y++ {
		for x := 0; x < m.Width; x++ {
			m.Cells[y][x].Reset()
		}
	}
	m.Start.Kind = Start
	m.Finish.Kind = Finish
}

func (m *Maze) Dig(startCell *MazeCell) {
	stack := make([]*MazeCell, 0, m.Width*m.Height)
	stack = append(stack, startCell)

	for len(stack) > 0 {
		cell := stack[len(stack)-1]
		stack = stack[:len(stack)-1]

		walkable := 0
		for _, n := range cell.Neighbors {
			if n.Kind.IsWalkable() {
				walkable++
			}
		}

		if walkable != 1 {
			continue
		}

		cell.Kind = Space

		for _, n := range cell.Neighbors {
			if n.Kind == Wall {
				stack = append(stack, n)
			}
		}
	}
}

func (m *Maze) EnsureOpenFinish(startCell *MazeCell) {
	stack := make([]*MazeCell, 0, m.Width*m.Height)
	stack = append(stack, startCell)

	for len(stack) > 0 {
		cell := stack[len(stack)-1]
		stack = stack[:len(stack)-1]

		cell.Kind = Space

		walkable := 0
		for _, n := range cell.Neighbors {
			if n.Kind.IsWalkable() {
				walkable++
			}
		}

		if walkable > 1 {
			continue
		}

		for _, n := range cell.Neighbors {
			if n.Kind == Wall {
				stack = append(stack, n)
			}
		}
	}
}

func (m *Maze) Generate() {
	for _, n := range m.Start.Neighbors {
		if n.Kind == Wall {
			m.Dig(n)
		}
	}

	for _, n := range m.Finish.Neighbors {
		if n.Kind == Wall {
			m.EnsureOpenFinish(n)
		}
	}
}

func (m *Maze) MiddleCell() *MazeCell {
	return m.Cells[m.Height/2][m.Width/2]
}

func (m *Maze) Checksum() uint32 {
	hasher := uint32(2166136261)
	prime := uint32(16777619)

	for y := 0; y < m.Height; y++ {
		for x := 0; x < m.Width; x++ {
			if m.Cells[y][x].Kind == Space {
				val := uint32(x * y)
				hasher = (hasher ^ val) * prime
			}
		}
	}
	return hasher
}

type MazeGenerator struct {
	BaseBenchmark
	width     int
	height    int
	maze      *Maze
	resultVal uint32
}

func (m *MazeGenerator) Prepare() {
	m.width = int(m.ConfigVal("w"))
	m.height = int(m.ConfigVal("h"))
	m.maze = NewMaze(m.width, m.height)
	m.maze.UpdateNeighbors()
	m.resultVal = 0
}

func (m *MazeGenerator) Run(iteration_id int) {
	m.maze.Reset()
	m.maze.Generate()
	m.resultVal += uint32(m.maze.MiddleCell().Kind)
}

func (m *MazeGenerator) Checksum() uint32 {
	return m.resultVal + m.maze.Checksum()
}

type MazeBFS struct {
	BaseBenchmark
	width     int
	height    int
	maze      *Maze
	resultVal uint32
	path      []*MazeCell
}

func (m *MazeBFS) Prepare() {
	m.width = int(m.ConfigVal("w"))
	m.height = int(m.ConfigVal("h"))
	m.maze = NewMaze(m.width, m.height)
	m.maze.UpdateNeighbors()
	m.maze.Generate()
	m.resultVal = 0
	m.path = nil
}

func (m *MazeBFS) bfs(start, target *MazeCell) []*MazeCell {
	if start == target {
		return []*MazeCell{start}
	}

	type PathNode struct {
		cell   *MazeCell
		parent int
	}

	visited := make([][]bool, m.height)
	for i := range visited {
		visited[i] = make([]bool, m.width)
	}

	queue := []int{0}
	pathNodes := []PathNode{{start, -1}}
	visited[start.Y][start.X] = true

	for len(queue) > 0 {
		pathId := queue[0]
		queue = queue[1:]
		cell := pathNodes[pathId].cell

		for _, neighbor := range cell.Neighbors {
			if neighbor == target {
				result := []*MazeCell{target}
				cur := pathId
				for cur >= 0 {
					result = append(result, pathNodes[cur].cell)
					cur = pathNodes[cur].parent
				}

				for i, j := 0, len(result)-1; i < j; i, j = i+1, j-1 {
					result[i], result[j] = result[j], result[i]
				}
				return result
			}

			if neighbor.Kind.IsWalkable() && !visited[neighbor.Y][neighbor.X] {
				visited[neighbor.Y][neighbor.X] = true
				pathNodes = append(pathNodes, PathNode{neighbor, pathId})
				queue = append(queue, len(pathNodes)-1)
			}
		}
	}
	return nil
}

func (m *MazeBFS) midCellChecksum(path []*MazeCell) uint32 {
	if len(path) == 0 {
		return 0
	}
	cell := path[len(path)/2]
	return uint32(cell.X * cell.Y)
}

func (m *MazeBFS) Run(iteration_id int) {
	m.path = m.bfs(m.maze.Start, m.maze.Finish)
	if m.path != nil {
		m.resultVal += uint32(len(m.path))
	}
}

func (m *MazeBFS) Checksum() uint32 {
	return m.resultVal + m.midCellChecksum(m.path)
}

type MazeAStar struct {
	BaseBenchmark
	width     int
	height    int
	maze      *Maze
	resultVal uint32
	path      []*MazeCell
}

func (m *MazeAStar) Prepare() {
	m.width = int(m.ConfigVal("w"))
	m.height = int(m.ConfigVal("h"))
	m.maze = NewMaze(m.width, m.height)
	m.maze.UpdateNeighbors()
	m.maze.Generate()
	m.resultVal = 0
	m.path = nil
}

func (m *MazeAStar) heuristic(a, b *MazeCell) int {
	return absint(a.X-b.X) + absint(a.Y-b.Y)
}

func absint(x int) int {
	if x < 0 {
		return -x
	}
	return x
}

func (m *MazeAStar) idx(y, x int) int {
	return y*m.width + x
}

func (m *MazeAStar) astar(start, target *MazeCell) []*MazeCell {
	if start == target {
		return []*MazeCell{start}
	}

	size := m.width * m.height

	cameFrom := make([]int, size)
	gScore := make([]int, size)
	bestF := make([]int, size)
	for i := 0; i < size; i++ {
		cameFrom[i] = -1
		gScore[i] = int(^uint(0) >> 1)
		bestF[i] = int(^uint(0) >> 1)
	}

	startIdx := m.idx(start.Y, start.X)
	targetIdx := m.idx(target.Y, target.X)

	type Item struct {
		priority int
		vertex   int
	}
	openSet := make([]Item, 0)

	gScore[startIdx] = 0
	fStart := m.heuristic(start, target)
	openSet = append(openSet, Item{fStart, startIdx})
	bestF[startIdx] = fStart

	for len(openSet) > 0 {

		minIdx := 0
		for i := 1; i < len(openSet); i++ {
			if openSet[i].priority < openSet[minIdx].priority {
				minIdx = i
			}
		}
		current := openSet[minIdx]

		openSet[minIdx] = openSet[len(openSet)-1]
		openSet = openSet[:len(openSet)-1]

		currentIdx := current.vertex

		if currentIdx == targetIdx {
			result := make([]*MazeCell, 0)
			cur := currentIdx
			for cur != -1 {
				y := cur / m.width
				x := cur % m.width
				result = append(result, m.maze.Cells[y][x])
				cur = cameFrom[cur]
			}

			for i, j := 0, len(result)-1; i < j; i, j = i+1, j-1 {
				result[i], result[j] = result[j], result[i]
			}
			return result
		}

		currentY := currentIdx / m.width
		currentX := currentIdx % m.width
		currentCell := m.maze.Cells[currentY][currentX]
		currentG := gScore[currentIdx]

		for _, neighbor := range currentCell.Neighbors {
			if !neighbor.Kind.IsWalkable() {
				continue
			}

			neighborIdx := m.idx(neighbor.Y, neighbor.X)
			tentativeG := currentG + 1

			if tentativeG < gScore[neighborIdx] {
				cameFrom[neighborIdx] = currentIdx
				gScore[neighborIdx] = tentativeG
				fNew := tentativeG + m.heuristic(neighbor, target)

				if fNew < bestF[neighborIdx] {
					bestF[neighborIdx] = fNew
					openSet = append(openSet, Item{fNew, neighborIdx})
				}
			}
		}
	}
	return nil
}

func (m *MazeAStar) midCellChecksum(path []*MazeCell) uint32 {
	if len(path) == 0 {
		return 0
	}
	cell := path[len(path)/2]
	return uint32(cell.X * cell.Y)
}

func (m *MazeAStar) Run(iteration_id int) {
	m.path = m.astar(m.maze.Start, m.maze.Finish)
	if m.path != nil {
		m.resultVal += uint32(len(m.path))
	}
}

func (m *MazeAStar) Checksum() uint32 {
	return m.resultVal + m.midCellChecksum(m.path)
}