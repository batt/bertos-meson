message('CPU: ' + cpu)
message('CPU frequency: @0@Hz'.format(cpu_freq))

cpu_attributes = {
    'STM32F207IG': { 'cflags': '-D__ARM_STM32F207IG__', 'core' : 'cortex-m3' },
    #add other CPUs here
}

if not (cpu in cpu_attributes)
    message('Unknown CPU!')
    message('List of supported CPUs:')
    foreach c, val : cpu_attributes
        message(c)
    endforeach
    error('Unknown CPU: ' + cpu)
endif

cpu_cflags = ['-DARCH=(ARCH_DEFAULT)', '-DWIZ_AUTOGEN']
cpu_cflags += '-DCPU_FREQ=@0@UL'.format(cpu_freq)
cpu_cflags += cpu_attributes[cpu]['cflags']

cpu_dep = declare_dependency(
    compile_args: cpu_cflags,
)

if 'cpu' in bertos_modules
    project_dependencies += declare_dependency(
        compile_args : bertos_cflags,
        dependencies : cpu_dep,
    )
endif


core = cpu_attributes[cpu]['core']
subdir(core)
