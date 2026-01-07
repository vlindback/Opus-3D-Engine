# game/meson.build

game_sources = files(
    'src/main.cpp',
    'src/game_application.cpp'
)

link_args = []

if host_machine.system() == 'windows'
    utf8_manifest = files('game_win32.manifest')
    link_args += [
        '/MANIFEST:EMBED',
        '/MANIFESTINPUT:@0@'.format(utf8_manifest[0].full_path()),
    ]
endif

executable(
    'Opus3D-Game',
    game_sources,
    dependencies: [engine_dep],
    link_args: link_args,
    include_directories: [
        '.',
        '..',
    ],
    install: true,
)
