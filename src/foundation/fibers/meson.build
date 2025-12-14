# foundation/fibers/meson.build

add_languages('masm', native: true)

cpu = host_machine.cpu_family()
sys = host_machine.system()
cpp = meson.get_compiler('cpp')

asm_sources = []

is_x64     = cpu == 'x86_64'
is_windows = sys == 'windows'
is_linux   = sys == 'linux'

if is_x64 and is_windows
  # Windows x64: choose MASM vs GAS/Clang
  if cpp.get_id() == 'msvc'
    asm_sources += files(
      'boost/asm/x86_64/win/make_x86_64_ms_pe_masm.masm',
      'boost/asm/x86_64/win/jump_x86_64_ms_pe_masm.masm',
      'boost/asm/x86_64/win/ontop_x86_64_ms_pe_masm.masm',
    )
  else
    # clang-cl / MinGW with GAS-style assembly
    asm_sources += files(
      'boost/asm/x86_64/win_clang/make_x86_64_ms_pe_clang_gas.S',
      'boost/asm/x86_64/win_clang/jump_x86_64_ms_pe_clang_gas.S',
      'boost/asm/x86_64/win_clang/ontop_x86_64_ms_pe_clang_gas.S',
    )
  endif
elif is_x64 and is_linux
  # Linux x86_64: SysV ELF
  asm_sources += files(
    'boost/asm/x86_64/sysv/make_x86_64_sysv_elf_gas.S',
    'boost/asm/x86_64/sysv/jump_x86_64_sysv_elf_gas.S',
    'boost/asm/x86_64/sysv/ontop_x86_64_sysv_elf_gas.S',
  )
else
  error('fibers: only x86_64 Windows and Linux are supported right now (got ' + sys + '/' + cpu + ')')
endif

sources = []
sources += files('boost/fcontext.cpp')
sources += asm_sources

fibers_lib = static_library(
  'fibers',
  sources,
  include_directories: include_directories('boost'),
)

# Export to the foundation module list
foundation_modules += fibers_lib
