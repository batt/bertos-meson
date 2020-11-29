
core_cflags = ['-mcpu=cortex-m3', '-g3', '-gdwarf-2', '-mthumb', '-mno-thumb-interwork',
                '-fno-strict-aliasing', '-fwrapv', '-fverbose-asm']

core_ldflags = ['-mthumb', '-mno-thumb-interwork', '-nostartfiles', '-Wl,--no-warn-mismatch']

core_sources = ['drv/irq_cm3.c',]
core_app_sources = ['hw/crt_cm3.S', 'hw/vectors_cm3.S', 'hw/init_cm3.c']

linker_scripts_flags = {
    'STM32F207IG': '-L@0@ -Tbertos/cpu/cortex-m3/scripts/stm32f207ig_rom.ld',
}

linker_script = []
if cpu in linker_scripts_flags
    format_str = linker_scripts_flags[cpu]
    linker_script_str = format_str.format(join_paths(bertos_path, '../'))
    linker_script = linker_script_str.split()
    message('Using linker script flags: @0@'.format(linker_script))
endif

stm32_conf = {
    'cflags' :  [],
    'sources' : ['drv/gpio_stm32.c', 'drv/clock_stm32.c'],
    'ldflags' : [],
}

conf = get_variable('@0@_conf'.format(cpu_family))

cpu_core_ldflags = core_ldflags + conf['ldflags'] + linker_script
cpu_core_inc = include_directories('.')
cpu_core_cflags = core_cflags + conf['cflags']

cpu_core_app_dep = declare_dependency(
    sources : files(core_app_sources),
    compile_args : cpu_core_cflags,
    include_directories: cpu_core_inc,
)

cpu_core_dep = declare_dependency(
    sources : files(core_sources, conf['sources']),
    dependencies : cpu_core_app_dep,
    link_args: cpu_core_ldflags,
)

cpu_kern_dep = declare_dependency(
    sources: files('hw/switch_ctx_cm3.c')
)

subdir('drv')
