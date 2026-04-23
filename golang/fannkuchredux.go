package main

type Fannkuchredux struct {
	BaseBenchmark
	n      int64
	result uint32
}

func (f *Fannkuchredux) Prepare() {
	f.n = f.ConfigVal("n")
}

func (f *Fannkuchredux) fannkuchredux(n int) (int, int) {
	var perm1 [32]int
	for i := range perm1[:n] {
		perm1[i] = i
	}
	var perm [32]int
	var count [32]int
	maxFlipsCount := 0
	permCount := 0
	checksum := 0
	r := n

	for {
		for r > 1 {
			count[r-1] = r
			r--
		}

		copy(perm[:n], perm1[:n])
		flipsCount := 0

		k := perm[0]
		for k != 0 {

			i := 0
			j := k
			for i < j {
				perm[i], perm[j] = perm[j], perm[i]
				i++
				j--
			}
			flipsCount++
			k = perm[0]
		}

		if flipsCount > maxFlipsCount {
			maxFlipsCount = flipsCount
		}

		if permCount&1 == 0 {
			checksum += flipsCount
		} else {
			checksum -= flipsCount
		}

		for {
			if r == n {
				return checksum, maxFlipsCount
			}

			first := perm1[0]
			copy(perm1[:r], perm1[1:r+1])
			perm1[r] = first

			count[r]--
			if count[r] > 0 {
				break
			}
			r++
		}
		permCount++
	}
}

func (f *Fannkuchredux) Run(iteration_id int) {
	a, b := f.fannkuchredux(int(f.n))
	f.result += uint32(a)*100 + uint32(b)
}

func (f *Fannkuchredux) Checksum() uint32 {
	return f.result
}