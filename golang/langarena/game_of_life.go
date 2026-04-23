package LangArena

type Cell struct {
	Alive     bool
	NextState bool
	Neighbors []*Cell
}

func NewCell() *Cell {
	return &Cell{
		Neighbors: make([]*Cell, 0, 8),
	}
}

func (c *Cell) AddNeighbor(neighbor *Cell) {
	c.Neighbors = append(c.Neighbors, neighbor)
}

func (c *Cell) ComputeNextState() {
	aliveNeighbors := 0
	for _, n := range c.Neighbors {
		if n.Alive {
			aliveNeighbors++
		}
	}

	if c.Alive {
		c.NextState = aliveNeighbors == 2 || aliveNeighbors == 3
	} else {
		c.NextState = aliveNeighbors == 3
	}
}

func (c *Cell) Update() {
	c.Alive = c.NextState
}

type Grid struct {
	width  int
	height int
	cells  [][]*Cell
}

func NewGrid(width, height int) *Grid {
	cells := make([][]*Cell, height)
	for y := 0; y < height; y++ {
		cells[y] = make([]*Cell, width)
		for x := 0; x < width; x++ {
			cells[y][x] = NewCell()
		}
	}

	grid := &Grid{
		width:  width,
		height: height,
		cells:  cells,
	}
	grid.linkNeighbors()
	return grid
}

func (g *Grid) linkNeighbors() {
	for y := 0; y < g.height; y++ {
		for x := 0; x < g.width; x++ {
			cell := g.cells[y][x]

			for dy := -1; dy <= 1; dy++ {
				for dx := -1; dx <= 1; dx++ {
					if dx == 0 && dy == 0 {
						continue
					}

					ny := (y + dy + g.height) % g.height
					nx := (x + dx + g.width) % g.width

					cell.AddNeighbor(g.cells[ny][nx])
				}
			}
		}
	}
}

func (g *Grid) NextGeneration() {

	for _, row := range g.cells {
		for _, cell := range row {
			cell.ComputeNextState()
		}
	}

	for _, row := range g.cells {
		for _, cell := range row {
			cell.Update()
		}
	}
}

func (g *Grid) CountAlive() int {
	count := 0
	for _, row := range g.cells {
		for _, cell := range row {
			if cell.Alive {
				count++
			}
		}
	}
	return count
}

func (g *Grid) ComputeHash() uint32 {
	const (
		FNV_OFFSET_BASIS uint32 = 2166136261
		FNV_PRIME        uint32 = 16777619
	)

	hash := FNV_OFFSET_BASIS
	for _, row := range g.cells {
		for _, cell := range row {
			var alive uint32 = 0
			if cell.Alive {
				alive = 1
			}
			hash = (hash ^ alive) * FNV_PRIME
		}
	}
	return hash
}

type GameOfLife struct {
	BaseBenchmark
	grid *Grid
}

func (g *GameOfLife) Prepare() {
	width := int(g.ConfigVal("w"))
	height := int(g.ConfigVal("h"))
	g.grid = NewGrid(width, height)

	for _, row := range g.grid.cells {
		for _, cell := range row {
			if NextFloat(1.0) < 0.1 {
				cell.Alive = true
			}
		}
	}
}

func (g *GameOfLife) Run(iterationId int) {
	g.grid.NextGeneration()
}

func (g *GameOfLife) Checksum() uint32 {
	return g.grid.ComputeHash() + uint32(g.grid.CountAlive())
}
