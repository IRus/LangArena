from std.python import Python, PythonObject
from helper import Helper


struct ConfigEntry(Copyable):
    var name: String
    var fields: Dict[String, PythonObject]

    def __init__(out self, name: String):
        self.name = name
        self.fields = Dict[String, PythonObject]()


struct Config(Copyable):
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
