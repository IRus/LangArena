module csv_bench;

import benchmark;
import helper;
import std::io;
import std::core::mem;
import std::core::string;
import std::encoding::csv;

struct Point
{
	double x, y, z;
}

struct CsvParseData
{
	String csv_data;
	uint result_val;
	long rows;
}

fn String generate_csv_for_parsing(long rows)
{
	DString buffer;
	buffer.init(mem, (usz)(rows * 100 + 1));
	defer buffer.free();

	for (long i = 0; i < rows; i++)
	{
		char c   = (char)('A' + (i % 26));
		double x = helper::helper_next_float(1.0);
		double z = helper::helper_next_float(1.0);
		double y = helper::helper_next_float(1.0);

		String bool_str = (i % 2 == 0) ? "true" : "false";

		String line = string::tformat(
			"\"point %c\\n, \"\"%d\"\"\",%.10f,,%.10f,\"[%s\\n, %d]\",%.10f\n",
			c,
			(int)(i % 100),
			x,
			z,
			bool_str,
			(int)(i % 100),
			y
		);
		buffer.append(line);
	}

	String result = buffer.str_view();
	char* copy    = mem::alloc_array(char, result.len + 1);
	mem::copy(copy, (char*)result, result.len);
	copy[result.len] = 0;
	return ((ZString)copy).str_view();
}

fn void csv_parse_prepare(Benchmark* self)
{
	CsvParseData* data = (CsvParseData*)self.data;
	data.rows = helper::helper_config_i64(self.name, "rows");
	data.csv_data = generate_csv_for_parsing(data.rows);
	data.result_val = 0;
}

fn void csv_parse_run(Benchmark* self, int iteration_id)
{
	CsvParseData* data = (CsvParseData*)self.data;
	if (data.csv_data.len == 0) return;

	Point* points   = mem::alloc_array(Point, (usz)data.rows);
	int point_count = 0;

	Splitter line_splitter = data.csv_data.tokenize("\n");

	while (try line = line_splitter.next())
	{
		if (line.len == 0 || point_count >= (int)data.rows) continue;

		double x       = 0;
		double y       = 0;
		double z       = 0;
		int field_idx  = 0;
		usz start      = 0;
		bool in_quotes = false;

		for (usz p = 0; p < line.len && field_idx < 6; p++)
		{
			char ch = line[p];
			if (ch == '"')
			{
				in_quotes = !in_quotes;
			}
			else if (ch == ',' && !in_quotes)
			{
				if (field_idx == 1 || field_idx == 3 || field_idx == 5)
				{
					String field = line[start:p - start];

					if (field.len >= 2 && field[0] == '"' && field[^1] == '"')
					{
						field = field[1:field.len - 2];
					}
					double val = field.to_double() ?? 0;
					if (field_idx == 1) x = val;
					if (field_idx == 3) z = val;
					if (field_idx == 5) y = val;
				}
				field_idx++;
				start = p + 1;
			}
		}

		if (field_idx == 5 && start < line.len)
		{
			String field = line[start:line.len - start];
			if (field.len >= 2 && field[0] == '"' && field[^1] == '"')
			{
				field = field[1:field.len - 2];
			}
			y = field.to_double() ?? 0;
			field_idx++;
		}

		if (field_idx >= 6)
		{
			points[point_count].x = x;
			points[point_count].y = y;
			points[point_count].z = z;
			point_count++;
		}
	}

	if (point_count > 0)
	{
		double x_sum = 0;
		double y_sum = 0;
		double z_sum = 0;
		for (int i = 0; i < point_count; i++)
		{
			x_sum += points[i].x;
			y_sum += points[i].y;
			z_sum += points[i].z;
		}

		double len = (double)point_count;
		data.result_val += helper::helper_checksum_f64(
			x_sum / len
		) + helper::helper_checksum_f64(y_sum / len) + helper::helper_checksum_f64(z_sum / len);
	}

	mem::free(points);
}

fn uint csv_parse_checksum(Benchmark* self)
{
	CsvParseData* data = (CsvParseData*)self.data;
	return data.result_val;
}

fn void csv_parse_cleanup(Benchmark* self)
{
	CsvParseData* data = (CsvParseData*)self.data;
	if (data.csv_data.ptr)
	{
		mem::free((void*)data.csv_data.ptr);
		data.csv_data = { };
	}
}

fn Benchmark* csv_parse_create() @cname("CsvParse_create") @export
{
	Benchmark* bench = benchmark::benchmark_create("CSV::Parse");

	CsvParseData* data = mem::new(CsvParseData);

	bench.data = data;
	bench.prepare = (PrepareFn)&csv_parse_prepare;
	bench.run = (RunFn)&csv_parse_run;
	bench.checksum = (ChecksumFn)&csv_parse_checksum;
	bench.cleanup = (CleanupFn)&csv_parse_cleanup;

	return bench;
}