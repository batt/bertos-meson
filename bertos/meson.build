project('BeRTOS', 'c', default_options: ['optimization=s', 'debug=true', 'b_pie=false', 'b_staticpic=false'])

cpu = get_option('cpu')

if cpu == ''
    error('CPU not set')
endif

cpu_freq = get_option('cpu_frequency')
debug = get_option('debug')

app_dir = get_option('app_dir')

bertos_modules = get_option('modules')
#Base CPU definition module is always needed
bertos_modules += 'cpu'

message('Active BeRTOS modules: @0@'.format(bertos_modules))

bertos_path = meson.current_source_dir()

optflags = ['-ffunction-sections', '-fdata-sections', '-fwrapv', '-fno-strict-aliasing']
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

ldflags = ['-Wl,--gc-sections']

bertos_debugflags = []

if debug
    bertos_debugflags += '-D_DEBUG'
    bertos_modules += ['kdebug', 'formatwr']
endif


bertos_cflags = bertos_debugflags + optflags + debugcflags + warnflags + c_warnflags + listflags + c_compiler_std

cpcfg = find_program('cpcfg.py')
cfg_src = 'cfg_src.h'
app_cfg = custom_target('gen-cfg',
    input : app_dir,
    output : cfg_src,
    command : [cpcfg, '@INPUT@', '@OUTPUT@'],
    build_by_default : true
)

# cpu dir must be included first because it contains
# cpu driver dependencies definitions.
subdir('cpu')

subdir('algo')
subdir('io')
subdir('drv')
subdir('kern')
subdir('mware')
subdir('net')

# Add dependencies for the enabled modules
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
    sources: files('verstag.c'),
    c_args: bertos_cflags,
    include_directories: include_dir,
    install: false,
    dependencies: module_dependencies,
)

bertos_dep = declare_dependency(
    compile_args: bertos_debugflags,
    include_directories: include_dir,
    link_with: bertos,
    link_args: ldflags,
    dependencies : application_dependencies,
)