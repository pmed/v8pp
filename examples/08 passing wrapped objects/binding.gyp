{
	'targets':
	[
		{
			'target_name': 'addon',
			'include_dirs': ['../..'],
			'cflags_cc': ['-std=c++11', '-fexceptions'],
			'msvs_settings': { 'VCCLCompilerTool': { 'ExceptionHandling': 1 }, 'VCLinkerTool': { 'AdditionalOptions': ['/FORCE:MULTIPLE'] } },
			'xcode_settings': { 'GCC_ENABLE_CPP_EXCEPTIONS': 'YES' },
			'sources': ['addon.cc', 'myobject.cc'],
		}
	]
}
