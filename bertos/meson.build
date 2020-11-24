project('BeRTOS', 'c')

bertos_modules = get_option('modules')
cpu = get_option('cpu')
cpu_freq = get_option('cpu_frequency')
debug = get_option('debug')

app_dir = get_option('app_dir')

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
    bertos_modules += 'kdebug'
endif

cpcfg = find_program('cpcfg.py')
cfg_src = 'cfg_src.h'
app_cfg = custom_target('gen-cfg',
    input : app_dir,
    output : cfg_src,
    command : [cpcfg, '@INPUT@', '@OUTPUT@'],
    build_by_default : true
)

subdir('cpu')

subdir('algo')
subdir('io')
subdir('drv')
subdir('kern')
subdir('mware')
subdir('net')

# Add dependancies for the enabled modules
module_dependencies = []
application_dependencies = []
foreach module : bertos_modules
    mod_dep_name = '@0@_dep'.format(module)
    if not is_variable(mod_dep_name)
        error('Unknown module \'' + module +'\'')
    endif
    module_dependencies += get_variable(mod_dep_name)

    app_dep_name = '@0@_app_dep'.format(module)
    if is_variable(app_dep_name)
        application_dependencies += get_variable(app_dep_name)
    endif
endforeach

include_dir = include_directories('.')

bertos = static_library('bertos', app_cfg,
    c_args: bertos_cflags,
    link_args: ldflags,
    include_directories: include_dir,
    install: false,
    dependencies: module_dependencies,
)

bertos_dep = declare_dependency(
    include_directories: include_dir,
    link_with: bertos,
    dependencies : application_dependencies,
)