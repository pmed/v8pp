#include <v8pp/module.hpp>
#include <v8pp/class.hpp>

#include <fstream>
#include <boost/filesystem/operations.hpp>

namespace file {

bool rename(char const* src, char const* dest)
{
	boost::system::error_code err;
	boost::filesystem::rename(src, dest, err);
	return !err;
}

bool mkdir(char const* name)
{
	boost::system::error_code err;
	boost::filesystem::create_directories(name, err);
	return !err;
}

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

class file_writer : file_base
{
public:
	explicit file_writer(v8::Arguments const& args)
	{
		if ( args.Length() == 1)
		{
			v8::String::Utf8Value str(args[0]);
			open(*str);
		}
	}

	bool open(char const* path)
	{
		stream_.open(path, std::ios_base::out);
		return stream_.good();
	}

	void print(v8::Arguments const& args)
	{
		for (int i = 0; i < args.Length(); ++i)
		{
			v8::HandleScope scope;
			if (i > 0) stream_ << ' ';
			v8::String::Utf8Value str(args[i]);
			stream_ << *str;
		}
	}

	void println(v8::Arguments const& args)
	{
		print(args);
		stream_ << std::endl;
	}
};

class file_reader : file_base
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

	v8::Handle<v8::Value> getline()
	{
		if ( stream_.good() && ! stream_.eof())
		{
			std::string line;
			std::getline(stream_, line);
			return v8pp::to_v8(line);
		}
		else
		{
			return v8::Undefined();
		}
	}
};

v8::Handle<v8::Value> init()
{
	v8::HandleScope scope;

	// no_factory argument disallow object creation in JavaScript
	v8pp::class_<file_base, v8pp::no_factory> file_base_class;
	file_base_class
		.set("close", &file_base::close)
		.set("good", &file_base::good)
		.set("is_open", &file_base::is_open)
		.set("eof", &file_base::eof)
		;

	// file_writer inherits from file_base_class
	// Second template argument is a factory. v8_args_factory passes
	// the v8::Arguments directly to constructor of file_writer
	v8pp::class_<file_writer, v8pp::v8_args_factory> file_writer_class(file_base_class);
	file_writer_class
		.set("open", &file_writer::open)
		.set("print", &file_writer::print)
		.set("println", &file_writer::println)
		;

	// file_reader also inherits from file_base
	// This factory calls file_reader(char const* ) constructor.
	// It converts v8::Arguments to the appropriate C++ arguments.
	v8pp::class_< file_reader, v8pp::factory<char const*> > file_reader_class(file_base_class);
	file_reader_class
		.set("open", &file_reader::open)
		.set("getln", &file_reader::getline)
		;

	// Create a module to add classes and functions to and return a
	// new instance of the module to be embedded into the v8 context
	v8pp::module m;
	m.set("rename", rename)
	 .set("mkdir", mkdir)
	 .set("writer", file_writer_class)
	 .set("reader", file_reader_class)
		;

	return scope.Close(m.new_instance());
}

} // namespace file

V8PP_PLUGIN_INIT
{
	return file::init();
}
