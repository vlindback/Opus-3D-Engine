# foundation/application/meson.build

application_sources = files(
    'src/application.cpp'
)

local_inc_paths = ['include']

if host_machine.system() == 'windows'
    local_inc_paths += 'src/platform/windows'
    application_sources += [
        'src/platform/windows/win32_application.cpp',
        'src/platform/windows/win32_input_backend.cpp'
    ]
elif host_machine.system() == 'linux'
    local_inc_paths += 'src/platform/linux'
    # ... add linux sources
endif

local_inc = include_directories(local_inc_paths)

application_lib = static_library(
  'application',
  [application_sources], # Meson flattens nested lists automatically
  include_directories: [local_inc, opus_src_root],
)

# Export to the foundation module list
foundation_modules += application_lib