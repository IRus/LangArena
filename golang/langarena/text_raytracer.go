package LangArena

import (
	"math"
)

type TextRaytracer struct {
	BaseBenchmark
	w, h   int32
	result uint32
}

type Vector struct {
	X, Y, Z float64
}

func (v Vector) Scale(s float64) Vector {
	return Vector{v.X * s, v.Y * s, v.Z * s}
}

func (v Vector) Add(other Vector) Vector {
	return Vector{v.X + other.X, v.Y + other.Y, v.Z + other.Z}
}

func (v Vector) Sub(other Vector) Vector {
	return Vector{v.X - other.X, v.Y - other.Y, v.Z - other.Z}
}

func (v Vector) Dot(other Vector) float64 {
	return v.X*other.X + v.Y*other.Y + v.Z*other.Z
}

func (v Vector) Magnitude() float64 {
	return math.Sqrt(v.Dot(v))
}

func (v Vector) Normalize() Vector {
	mag := v.Magnitude()
	if mag == 0.0 {
		return Vector{0, 0, 0}
	}
	return v.Scale(1.0 / mag)
}

type Ray struct {
	Orig, Dir Vector
}

type Color struct {
	R, G, B float64
}

func (c Color) Scale(s float64) Color {
	return Color{c.R * s, c.G * s, c.B * s}
}

func (c Color) Add(other Color) Color {
	return Color{c.R + other.R, c.G + other.G, c.B + other.B}
}

type Sphere2 struct {
	Center Vector
	Radius float64
	Color  Color
}

func (s Sphere2) GetNormal(pt Vector) Vector {
	return pt.Sub(s.Center).Normalize()
}

type Light2 struct {
	Position Vector
	Color    Color
}

var (
	WHITE2  = Color{1.0, 1.0, 1.0}
	RED2    = Color{1.0, 0.0, 0.0}
	GREEN2  = Color{0.0, 1.0, 0.0}
	BLUE2   = Color{0.0, 0.0, 1.0}
	LIGHT12 = Light2{Vector{0.7, -1.0, 1.7}, WHITE2}
	LUT     = []byte{'.', '-', '+', '*', 'X', 'M'}
)

var SCENE2 = []Sphere2{
	{Vector{-1.0, 0.0, 3.0}, 0.3, RED2},
	{Vector{0.0, 0.0, 3.0}, 0.8, GREEN2},
	{Vector{1.0, 0.0, 3.0}, 0.4, BLUE2},
}

func (t *TextRaytracer) Prepare() {
	t.w = int32(t.ConfigVal("w"))
	t.h = int32(t.ConfigVal("h"))
}

func (t *TextRaytracer) shadePixel(ray Ray, obj Sphere2, tval float64) int {
	pi := ray.Orig.Add(ray.Dir.Scale(tval))
	color := t.diffuseShading(pi, obj, LIGHT12)
	col := (color.R + color.G + color.B) / 3.0
	idx := int(col * 6.0)
	if idx < 0 {
		idx = 0
	}
	if idx >= 6 {
		idx = 5
	}
	return idx
}

func (t *TextRaytracer) intersectSphere(ray Ray, center Vector, radius float64) (float64, bool) {
	l := center.Sub(ray.Orig)
	tca := l.Dot(ray.Dir)
	if tca < 0.0 {
		return 0, false
	}

	d2 := l.Dot(l) - tca*tca
	r2 := radius * radius
	if d2 > r2 {
		return 0, false
	}

	thc := math.Sqrt(r2 - d2)
	t0 := tca - thc
	if t0 > 10000.0 {
		return 0, false
	}

	return t0, true
}

func (t *TextRaytracer) clamp(x, a, b float64) float64 {
	if x < a {
		return a
	}
	if x > b {
		return b
	}
	return x
}

func (t *TextRaytracer) diffuseShading(pi Vector, obj Sphere2, light Light2) Color {
	n := obj.GetNormal(pi)
	lightDir := light.Position.Sub(pi).Normalize()
	lam1 := lightDir.Dot(n)
	lam2 := t.clamp(lam1, 0.0, 1.0)
	return light.Color.Scale(lam2 * 0.5).Add(obj.Color.Scale(0.3))
}

func (t *TextRaytracer) Run(iteration_id int) {
	fw := float64(t.w)
	fh := float64(t.h)

	for j := int32(0); j < t.h; j++ {
		for i := int32(0); i < t.w; i++ {
			fi := float64(i)
			fj := float64(j)

			ray := Ray{
				Orig: Vector{0.0, 0.0, 0.0},
				Dir:  Vector{(fi - fw/2.0) / fw, (fj - fh/2.0) / fh, 1.0}.Normalize(),
			}

			var tval float64
			var hitObj Sphere2
			found := false

			for idx := 0; idx < len(SCENE2); idx++ {
				if inter, ok := t.intersectSphere(ray, SCENE2[idx].Center, SCENE2[idx].Radius); ok {
					tval = inter
					hitObj = SCENE2[idx]
					found = true
					break
				}
			}

			pixel := byte(' ')
			if found {
				idx := t.shadePixel(ray, hitObj, tval)
				pixel = LUT[idx]
			}

			t.result += uint32(pixel)
		}
	}
}

func (t *TextRaytracer) Checksum() uint32 {
	return t.result
}