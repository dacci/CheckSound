{
  'includes': [
    '../../build/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'lib1770-2',
      'type': 'static_library',
      'include_dirs': [
        '.'
      ],
      'msbuild_settings': {
        'ClCompile': {
          'DisableSpecificWarnings': '4200;4244;4245',
        },
      },
      'sources': [
        'lib1770.h',
        'lib1770_biquad.c',
        'lib1770_block.c',
        'lib1770_pre.c',
        'lib1770_stats.c',
      ],
    },
  ],
}
