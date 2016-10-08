{
    'variables': {
        'component': 'shared_library',
        'werror': '',
        'v8_use_snapshot': 'false',
        'v8_use_external_startup_data': 0,
    },
    'target_defaults': {
        'configurations': {
            'Debug': { 'msvs_settings':   { 'VCCLCompilerTool': { 'WarningLevel': '0', 'WarnAsError': 'false' } } },
            'Release': { 'msvs_settings': { 'VCCLCompilerTool': { 'WarningLevel': '0', 'WarnAsError': 'false' } } },
        },
        'msvs_cygwin_shell': 0,
        'target_conditions': [
            ['_type=="static_library"', {
                'standalone_static_library': 1,
            }]
       ],
    },
}
