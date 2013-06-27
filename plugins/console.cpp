#include <iostream>
#include <v8pp/module.hpp>

namespace console {

v8::Handle<v8::Value> log(v8::Arguments const& args)
{
	for (int i = 0; i < args.Length(); ++i)
	{
		v8::HandleScope handle_scope;
		if (i > 0) std::cout << ' ';
		v8::String::Utf8Value str(args[i]);
		std::cout <<  *str;
	}
	std::cout << std::endl;
	return v8::Undefined();
}

v8::Handle<v8::Value> init()
{
	v8::HandleScope scope;

	v8pp::module m;
	m.set("log", log);

	return scope.Close(m.new_instance());
}

} // namespace console

V8PP_PLUGIN_INIT
{
	return console::init();
}
