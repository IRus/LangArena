package LangArena

import (
	"math"
)

type Planet struct {
	x, y, z    float64
	vx, vy, vz float64
	mass       float64
}

func NewPlanet(x, y, z, vx, vy, vz, mass float64) *Planet {
	return &Planet{
		x: x, y: y, z: z,
		vx:   vx * 365.24,
		vy:   vy * 365.24,
		vz:   vz * 365.24,
		mass: mass * 4 * math.Pi * math.Pi,
	}
}

func (p *Planet) MoveFromI(bodies []*Planet, dt float64, start int) {
	for i := start; i < len(bodies); i++ {
		b2 := bodies[i]
		dx := p.x - b2.x
		dy := p.y - b2.y
		dz := p.z - b2.z

		distance := math.Sqrt(dx*dx + dy*dy + dz*dz)
		mag := dt / (distance * distance * distance)
		bMassMag := p.mass * mag
		b2MassMag := b2.mass * mag

		p.vx -= dx * b2MassMag
		p.vy -= dy * b2MassMag
		p.vz -= dz * b2MassMag
		b2.vx += dx * bMassMag
		b2.vy += dy * bMassMag
		b2.vz += dz * bMassMag
	}

	p.x += dt * p.vx
	p.y += dt * p.vy
	p.z += dt * p.vz
}

type Nbody struct {
	BaseBenchmark
	body []*Planet
	v1   float64
}

func (n *Nbody) Prepare() {
	n.body = []*Planet{
		NewPlanet(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0),
		NewPlanet(
			4.84143144246472090e+00,
			-1.16032004402742839e+00,
			-1.03622044471123109e-01,
			1.66007664274403694e-03,
			7.69901118419740425e-03,
			-6.90460016972063023e-05,
			9.54791938424326609e-04,
		),
		NewPlanet(
			8.34336671824457987e+00,
			4.12479856412430479e+00,
			-4.03523417114321381e-01,
			-2.76742510726862411e-03,
			4.99852801234917238e-03,
			2.30417297573763929e-05,
			2.85885980666130812e-04,
		),
		NewPlanet(
			1.28943695621391310e+01,
			-1.51111514016986312e+01,
			-2.23307578892655734e-01,
			2.96460137564761618e-03,
			2.37847173959480950e-03,
			-2.96589568540237556e-05,
			4.36624404335156298e-05,
		),
		NewPlanet(
			1.53796971148509165e+01,
			-2.59193146099879641e+01,
			1.79258772950371181e-01,
			2.68067772490389322e-03,
			1.62824170038242295e-03,
			-9.51592254519715870e-05,
			5.15138902046611451e-05,
		),
	}
	n.offsetMomentum()
	n.v1 = n.energy()
}

func (n *Nbody) offsetMomentum() {
	px, py, pz := 0.0, 0.0, 0.0

	for _, b := range n.body {
		px += b.vx * b.mass
		py += b.vy * b.mass
		pz += b.vz * b.mass
	}

	b := n.body[0]
	b.vx = -px / (4 * math.Pi * math.Pi)
	b.vy = -py / (4 * math.Pi * math.Pi)
	b.vz = -pz / (4 * math.Pi * math.Pi)
}

func (n *Nbody) energy() float64 {
	e := 0.0
	nbodies := len(n.body)

	for i := 0; i < nbodies; i++ {
		b := n.body[i]
		e += 0.5 * b.mass * (b.vx*b.vx + b.vy*b.vy + b.vz*b.vz)
		for j := i + 1; j < nbodies; j++ {
			b2 := n.body[j]
			dx := b.x - b2.x
			dy := b.y - b2.y
			dz := b.z - b2.z
			distance := math.Sqrt(dx*dx + dy*dy + dz*dz)
			e -= (b.mass * b2.mass) / distance
		}
	}
	return e
}

func (n *Nbody) Run(iteration_id int) {
	for k := 0; k < 1000; k += 1 {
		for i, b := range n.body {
			b.MoveFromI(n.body, 0.01, i+1)
		}
	}
}

func (n *Nbody) Checksum() uint32 {
	v2 := n.energy()
	return (ChecksumFloat64(n.v1) << 5) & ChecksumFloat64(v2)
}