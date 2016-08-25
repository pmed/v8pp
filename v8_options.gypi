{
    'variables': {
        'component': 'shared_library',
        'werror': '',
        'v8_use_snapshot': 'false',
        'v8_use_external_startup_data': 0,
    },
    'target_defaults': {
        'msvs_disabled_warnings': [ 4251],  # class 'std::xx' needs to have dll-interface.
        'msvs_cygwin_shell': 0,
        'target_conditions': [
            ['_type=="static_library"', {
                'standalone_static_library': 1,
            }]
       ],
    },
}
