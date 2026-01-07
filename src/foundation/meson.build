# foundation/meson.build

# --- Subdirectories ---
# These subdirs should append their library objects to foundation_modules

foundation_modules = []
foundation_inc = [include_directories('.'), opus_src_root]

subdir('core')
subdir('containers')
subdir('fibers')
subdir('memory')
subdir('application')

foundation_lib = static_library(
    'foundation',
    [],
    link_whole: foundation_modules,
    include_directories: foundation_inc,
    cpp_args: '-D_OPUS_FOUNDATION_BUILD',
    install: true,
)

# --- Foundation as a dependency ---
foundation_dep = declare_dependency(
    link_with: foundation_lib,
    # Crucial: Anyone using foundation needs to see the 'src' root
    # so they can resolve <foundation/module/include/header.hpp>
    include_directories: foundation_inc,
)