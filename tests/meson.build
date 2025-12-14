# tests/meson.build

test_framework_sources = files(
    'test_framework.cpp',
)

unit_test_sources = files(
    'main.cpp',
    'unit_tests/foundation/fiber_tests.cpp'
)

opus_test_exe = executable(
    'Opus3D-Test',
    test_framework_sources + unit_test_sources,
    dependencies: [engine_dep],
    include_directories: [
        '.',
        '..',
    ],
    install: true
)

test('opus3d', opus_test_exe)