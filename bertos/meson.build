project('BeRTOS', 'c')

bertos_modules = get_option('modules')
cpu = get_option('cpu')
cpu_freq = get_option('cpu_frequency')
debug = get_option('debug')
project_hw_dir = include_directories('../../.')

bertos_path = meson.current_source_dir()

message('Active BeRTOS modules: @0@'.format(bertos_modules))

optflags = ['-ffunction-sections', '-fdata-sections']
debugcflags = ['-ggdb']
warnflags = [
	'-W', '-Wformat', '-Wall', '-Wundef', '-Wpointer-arith', '-Wcast-qual',
	'-Wcast-align', '-Wwrite-strings', '-Wsign-compare',
	'-Wmissing-noreturn',
	'-Wextra', '-Wstrict-aliasing=2',
]
c_warnflags = [
	'-Wmissing-prototypes', '-Wstrict-prototypes',
	'-Wno-cast-function-type',
]
listflags = ['-dp'] ##-Wa,-anhlmsd=$(@:.o=.lst)
c_compiler_std = ['-std=gnu99']

MAP_FLAGS = [] ##-Wl,-Map=$(@:%.elf=%.map),--cref

ldflags = ['-Wl,--gc-sections']

bertos_cflags = optflags + debugcflags + warnflags + c_warnflags + listflags + c_compiler_std

if debug
    bertos_cflags += '-D_DEBUG'
endif

project_dependencies = []
subdir('cpu')
subdir('net')


# Add dependancies for the enabled modules
unknown_module = declare_dependency()
module_dependencies = []

foreach module : bertos_modules
    dep = get_variable('@0@_dep'.format(module), unknown_module)
    if dep == unknown_module
        error('Unknown module \'' + module +'\'')
    endif
    module_dependencies += dep
endforeach

include_dir = include_directories('.', 'hw')

bertos = static_library('bertos',
    c_args: bertos_cflags,
    link_args: ldflags,
    include_directories: [project_hw_dir, include_dir],
    install: false,
    dependencies: module_dependencies,
)

bertos_dep = declare_dependency(
    include_directories: include_dir,
    link_with: bertos,
    dependencies : project_dependencies,
)