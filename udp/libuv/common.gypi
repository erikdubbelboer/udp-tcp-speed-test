{
  'variables': {
    'library%': 'static_library',    # allow override to 'shared_library' for DLL/.so builds
  },

  'target_defaults': {
    'default_configuration': 'Release',
    'configurations': {
      'Debug': {
        'defines': [ 'DEBUG', '_DEBUG' ],
        'cflags': [ '-g', '-O0' ]
      },
      'Release': {
        'defines': [ 'NDEBUG' ],
        'cflags': [ '-O2', '-fno-strict-aliasing' ],
      }
    },
    
    'cflags'   : [ '-Wall', '-Wextra', '-pedantic', '-pthread', '-fPIC', '-std=c99' ],
    'cflags_cc': [ '-fno-rtti', '-std=c++0x' ],
    'ldflags'  : [ '-pthread', '-rdynamic' ],

    'conditions': [
      [ 'target_arch=="x64"', {
        'cflags': [ '-m64' ],
        'ldflags': [ '-m64' ],
      }],
    ]
  }
}

