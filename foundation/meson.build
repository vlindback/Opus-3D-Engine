foundation_inc = include_directories('include')

foundation_sources = files(
    'src/memory/*.cpp',
    'src/containers/*.cpp',
    'src/filesystem/*.cpp',
    'src/threading/*.cpp',
    'src/input/*.cpp',
)

# Platform-specific block
if host_machine.system() == 'windows'
    foundation_sources += files('src/platform/win32/*.cpp')
elif host_machine.system() == 'linux'
    foundation_sources += files('src/platform/linux/*.cpp')
elif host_machine.system() == 'darwin'
    foundation_sources += files('src/platform/mac/*.mm', 'src/platform/mac/*.cpp')
endif

foundation_lib = library(
    'foundation',
    foundation_sources,
    include_directories: foundation_inc,
    dependencies: [ /* dependencies to external libs */ ]
)

foundation_dep = declare_dependency(
    link_with: foundation_lib,
    include_directories: foundation_inc
)
