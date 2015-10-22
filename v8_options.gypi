{
    'variables': {
        'component': 'shared_library',
        'werror': '',
        'v8_use_snapshot': 'false',
        'v8_use_external_startup_data': 0,
    },
    'target_defaults': {
        'target_conditions': [
            ['_type=="static_library"', {
                'standalone_static_library': 1,
            }]
       ],
    },
}
