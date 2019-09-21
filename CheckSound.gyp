{
  'includes': [
    'build/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'CheckSound',
      'type': 'none',
      'dependencies': [
        'chksound/chksound.gyp:chksound',
        'third_party/lib1770-2/lib1770.gyp:lib1770-2',
      ],
    },
  ],
}
