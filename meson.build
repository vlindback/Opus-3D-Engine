project(
    'Opus3D',
    'cpp', 'masm', 
    version: '0.1.0',
    default_options: [
	'cpp_std=c++latest',
	'b_vscrt=md',
    ],
)

subdir('src/foundation')
subdir('src/engine')

app_target = get_option('app_target')

if app_target == 'Editor'
    subdir('src/editor')
elif app_target == 'Game'
    subdir('src/game')
else
    error('Unknown app target: ' + app_target)
endif