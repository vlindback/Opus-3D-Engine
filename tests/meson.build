# tests/meson.build

test_framework_sources = files(
    'test_framework.cpp',
)

unit_test_sources = files(
    'main.cpp',
    'unit_tests/foundation/fiber_tests.cpp',
    'unit_tests/foundation/container_tests.cpp'
)

link_args = []

if host_machine.system() == 'windows'
    utf8_manifest = files('tests_win32.manifest')
    link_args += [
        '/MANIFEST:EMBED',
        '/MANIFESTINPUT:@0@'.format(utf8_manifest[0].full_path()),
    ]
endif

opus_test_exe = executable(
    'Opus3D-Test',
    test_framework_sources + unit_test_sources,
    dependencies: [engine_dep],
    include_directories: [
        '.',
        '..',
        '../src/'
    ],
    link_args: link_args,
    install: true
)

test('opus3d', opus_test_exe)