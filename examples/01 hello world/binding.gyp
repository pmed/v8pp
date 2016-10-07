{
	'targets':
	[
		{
			'target_name': 'addon',
			'include_dirs': ['../..'],
			'cflags_cc': ['-std=c++11', '-fexceptions', '-frtti'],
			'sources': ['hello.cc'],
		}
	]
}