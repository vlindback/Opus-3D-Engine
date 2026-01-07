# OPUS3D/meson.build

project(
    'Opus3D',
    'cpp', 'masm', 
    version: '0.1.0',
    default_options: [
	    'cpp_std=c++latest',
	    'b_vscrt=md',
    ],
)

# This points to C:\code\opus3d\src
# Any subproject using this will correctly resolve <foundation/...>
opus_src_root = include_directories('src')

subdir('src/foundation')
subdir('src/engine')
subdir('src/editor')
subdir('src/game')
subdir('tests')