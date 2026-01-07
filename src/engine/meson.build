# engine/meson.build

# --- Dependencies ---
foundation_dep = get_variable('foundation_dep')

# Add other dependencies here (e.g., vulkan, win32, etc. if required directly)

# --- Source Files ---
engine_sources = files(
    'core/src/application.cpp',
    'core/src/engine.cpp'
)


# --- Include Paths ---

engine_includes = include_directories(
    '.', # Includes 'engine/' itself (for things like engine/core/systems/...)
    '..',
)


# --- Engine Library Target ---.
engine_lib = static_library(
    'engine',
    engine_sources,
    dependencies: [foundation_dep],
    include_directories: engine_includes,
    install: true,
)


# --- Engine as a dependency ---
engine_dep = declare_dependency(
    link_with: engine_lib,
    include_directories: engine_includes,
)

set_variable('engine_dep', engine_dep)