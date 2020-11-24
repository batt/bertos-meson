message('CPU: ' + cpu)
message('CPU frequency: @0@Hz'.format(cpu_freq))

cpu_attributes = {
    'STM32F207IG': { 'cflags': '-D__ARM_STM32F207IG__', 'core' : 'cortex-m3', 'family' : 'stm32' },
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

cpu_app_dep = declare_dependency(
    compile_args : cpu_cflags,
)

cpu_dep = declare_dependency(
    compile_args: cpu_cflags,
)

cpu_core = cpu_attributes[cpu]['core']
cpu_family = cpu_attributes[cpu]['family']
subdir(cpu_core)
