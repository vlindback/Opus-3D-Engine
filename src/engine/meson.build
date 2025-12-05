# engine/meson.build

# --- 1. Find Dependencies ---
# Assuming 'foundation' is defined in the root meson.build or a subproject
foundation_dep = get_variable('foundation_dep')
# Add other dependencies here (e.g., vulkan, win32, etc. if required directly)

# --- 2. Define Source Files ---
# List every source file using its path relative to this 'engine/' directory.
# Meson automatically finds .hpp and other header files when it compiles .cpp files.
engine_sources = files(
    # Core systems
    #'core/systems/task_system/task_system.cpp',
    #'core/systems/asset_system/asset_system.cpp',

    # Other engine files
    #'render/renderer.cpp',
    #'input/input_manager.cpp',
    # ... more files

    'core/application.cpp',
    'core/engine.cpp'
)


# --- 3. Define Include Paths ---
# You need to tell the compiler where to look for the headers (the .hpp files).
# Since your headers are right next to their source files, you just need to
# include the root engine directory and its subdirectories.

engine_includes = include_directories(
    '.', # Includes 'engine/' itself (for things like engine/core/systems/...)
    '..',
    #'core/systems/task_system',
    #'core/systems/asset_system',
    # Add ALL directories that contain headers you intend to use with #include <...>
)


# --- 4. Define the Engine Library Target ---

# Define the main engine library (static or shared).
engine_lib = static_library(
    'engine',
    engine_sources,
    # Link against your external dependencies
    dependencies: [foundation_dep],
    # Tell the compiler where to find the headers for this library
    include_directories: engine_includes,
)


# --- 5. Export the Library ---

# Export the compiled engine library as a dependency object.
engine_dep = declare_dependency(
    link_with: engine_lib,
    include_directories: engine_includes,
)

set_variable('engine_dep', engine_dep)