from std.python import Python
from helper import Helper
from benchmark import Benchmark, Config


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
