{
	'targets':
	[
		{
			'target_name': 'addon',
			'include_dirs': ['../..'],
			'cflags_cc': ['-std=c++17', '-fexceptions'],
			'msvs_settings': { 'VCCLCompilerTool': { 'ExceptionHandling': 1, 'AdditionalOptions': ['/std:c++17'] } },
			'xcode_settings': { 'GCC_ENABLE_CPP_EXCEPTIONS': 'YES', 'OTHER_CFLAGS': ['-std=c++17'] },
			'sources': ['addon.cc'],
		}
	]
}
