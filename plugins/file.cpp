#include <v8pp/module.hpp>
#include <v8pp/class.hpp>
#include <v8pp/config.hpp>

#include <fstream>

namespace file {

class file_base
{
public:
	bool is_open() const { return stream_.is_open(); }
	bool good() const { return stream_.good(); }
	bool eof() const { return stream_.eof(); }
	void close() { stream_.close(); }

protected:
	std::fstream stream_;
};

class file_writer : public file_base
{
public:
	explicit file_writer(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		if (args.Length() == 1)
		{
			v8::String::Utf8Value const str(args.GetIsolate(), args[0]);
			open(*str);
		}
	}

	bool open(char const* path)
	{
		stream_.open(path, std::ios_base::out);
		return stream_.good();
	}

	void print(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		v8::HandleScope scope(args.GetIsolate());

		for (int i = 0; i < args.Length(); ++i)
		{
			if (i > 0) stream_ << ' ';
			v8::String::Utf8Value const str(args.GetIsolate(), args[i]);
			stream_ << *str;
		}
	}

	void println(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		print(args);
		stream_ << std::endl;
	}
};

class file_reader : public file_base
{
public:
	explicit file_reader(char const* path)
	{
		open(path);
	}

	bool open(const char* path)
	{
		stream_.open(path, std::ios_base::in);
		return stream_.good();
	}

	v8::Local<v8::Value> getline(v8::Isolate* isolate)
	{
		if (stream_.good() && !stream_.eof())
		{
			std::string line;
			std::getline(stream_, line);
			return v8pp::to_v8(isolate, line);
		}
		else
		{
			return v8::Undefined(isolate);
		}
	}
};

v8::Local<v8::Value> init(v8::Isolate* isolate)
{
	v8::EscapableHandleScope scope(isolate);

	// file_base binding, no .ctor() specified, object creation disallowed in JavaScript
	v8pp::class_<file_base> file_base_class(isolate);
	file_base_class
		.set("close", &file_base::close)
		.set("good", &file_base::good)
		.set("is_open", &file_base::is_open)
		.set("eof", &file_base::eof)
		;

	// .ctor<> template arguments declares types of file_writer constructor
	// file_writer inherits from file_base_class
	v8pp::class_<file_writer> file_writer_class(isolate);
	file_writer_class
		.ctor<v8::FunctionCallbackInfo<v8::Value> const&>()
		.inherit<file_base>()
		.set("open", &file_writer::open)
		.set("print", &file_writer::print)
		.set("println", &file_writer::println)
		;

	// .ctor<> template arguments declares types of file_reader constructor.
	// file_base inherits from file_base_class
	v8pp::class_<file_reader> file_reader_class(isolate);
	file_reader_class
		.ctor<char const*>()
		.inherit<file_base>()
		.set("open", &file_reader::open)
		.set("getln", &file_reader::getline)
		;

	// Create a module to add classes and functions to and return a
	// new instance of the module to be embedded into the v8 context
	v8pp::module m(isolate);
	m.set("rename", [](char const* src, char const* dest) -> bool
	{
		return std::rename(src, dest) == 0;
	});
	m.set("writer", file_writer_class);
	m.set("reader", file_reader_class);

	return scope.Escape(m.new_instance());
}

} // namespace file

V8PP_PLUGIN_INIT(v8::Isolate* isolate)
{
	return file::init(isolate);
}
