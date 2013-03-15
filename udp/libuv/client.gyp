{
  'targets': [
    {
      'target_name': 'client',
      'type': 'executable',

      'dependencies': [
        'deps/libuv/uv.gyp:libuv'
      ],

      'include_dirs': [
        'deps/libuv/include',
      ],

      'sources': [
        'client.c',
      ],

      'defines': [
        'ARCH="<(target_arch)"',
        'PLATFORM="<(OS)"'
      ]
    },
  ]
}

