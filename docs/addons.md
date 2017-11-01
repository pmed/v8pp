# Node.js addons

The library allows to create native C++ addons for [Node.js](http://nodejs.org/)
version 0.11 and above, and [io.js](https://iojs.org/) version 1.0.

See [Addons documentation](http://nodejs.org/docs/v0.12.0/api/addons.html)
for reference.

Node.js native addons uses `node-gyp`(https://github.com/nodejs/node-gyp)
build tool to make them with C++ compiler.

This library is published as an NPM package: https://www.npmjs.com/package/v8pp
and maybe used as an addon dependency in a way similar to NAN.

To use `v8pp` in a native Node.js addon append it as a dependency in the addon
`package.json` file:

```json
{
	"dependencies": {
		"v8pp": "^1.0"
	}
}
```

This library uses C++11 with exceptions which should be enabled by overriding
C++ compiler flags in `bindings.gyp` file:
```
{
	'targets':
	[
		{
			'target_name': 'addon',
			'cflags_cc': ['-std=c++11', '-fexceptions'],
			'msvs_settings': { 'VCCLCompilerTool': { 'ExceptionHandling': 1 } },
			'xcode_settings': { 'GCC_ENABLE_CPP_EXCEPTIONS': 'YES' },
			'defines!': ['V8_DEPRECATION_WARNINGS=1'],
			'include_dirs': [ '<!(node -e require(\'v8pp\'))'],
			'sources': ['hello.cc'],
		}
	]
}
```

Here is a side-by-side difference between Node and v8pp in amount of C++ code
required to implement similar addons:
https://github.com/pmed/v8pp/commit/34c6344bdb1bdf7f0b46db6ea15c8ddd20ac3824?diff=split
