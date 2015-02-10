# Node.js addons

The library allows to create native C++ addons for [Node.js](http://nodejs.org/) version 0.11 and above, and [io.js](https://iojs.org/) version 1.0. See [Addons documentation](http://nodejs.org/docs/v0.12.0/api/addons.html) for reference.

Additionaly C++11 support and exceptions should be enabled for C++ compiler flags in `bindings.gyp` file:
```
{
	'targets':
	[
		{
			'target_name': 'addon',
			'include_dirs': ['../..'],
			'cflags_cc': ['-std=c++11', '-fexceptions'],
			'sources': ['hello.cc'],
		}
	]
}
```

The difference between Node and v8pp addon bindings: https://github.com/pmed/v8pp/commit/34c6344bdb1bdf7f0b46db6ea15c8ddd20ac3824
