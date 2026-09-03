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
