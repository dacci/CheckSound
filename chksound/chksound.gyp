{
  'includes': [
    '../build/common.gypi',
  ],

  'targets': [
    {
      'target_name': 'chksound',
      'type': 'executable',

      'dependencies': [
        '../third_party/lib1770-2/lib1770.gyp:lib1770-2',
      ],

      'include_dirs': [
        '.',
        '../third_party/lib1770-2',
      ],

      'conditions': [

        ['OS=="mac"', {
          'defines': [
            'OSATOMIC_USE_INLINED=1',
          ],
          'libraries': [
            'libmpg123.dylib',
            'libtag.dylib',
          ],
        }],

        ['OS=="linux"', {
          'cflags': [
            '<!@(<(pkg-config) --cflags libmpg123)',
            '<!@(<(pkg-config) --cflags taglib)',
          ],
          'ldflags': [
            '<!@(<(pkg-config) --libs-only-L --libs-only-other libmpg123)',
            '<!@(<(pkg-config) --libs-only-L --libs-only-other taglib)',
          ],
          'libraries': [
            '-lstdc++fs',
            '-lpthread',
            '<!@(<(pkg-config) --libs-only-l libmpg123)',
            '<!@(<(pkg-config) --libs-only-l taglib)',
          ],
        }],

        ['OS=="win"', {
          'libraries': [
            'mfplat.lib',
            'mfreadwrite.lib',
            'mfuuid.lib',
            'ole32.lib',
            'tag.lib',
          ],
          'msbuild_settings': {
            'ClCompile': {
              'DisableSpecificWarnings': [
                '4245',
                '4251',
              ],
            },
          },
        }],

      ],

      'sources': [
        'app/analyzer.cc',
        'app/analyzer.h',
        'app/main.cc',
        'audio/audio_reader.h',
        'audio/audio_reader_linux.cc',
        'audio/audio_reader_mac.cc',
        'audio/audio_reader_win.cc',
        'audio/gain_analysis.h',
        'util/scoped_initialize.h',
        'util/scoped_initialize_linux.cc',
        'util/scoped_initialize_mac.cc',
        'util/scoped_initialize_win.cc',
      ],
    },
  ],
}
