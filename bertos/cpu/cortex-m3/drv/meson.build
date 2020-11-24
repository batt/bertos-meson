cpu_timer_dep = declare_dependency(
    sources: files('timer_cm3.c')
)

cpu_kdebug_dep = declare_dependency(
    sources: files('kdebug_cm3.c')
)

cpu_eth_dep = declare_dependency(
    sources: files('eth_@0@.c'.format(cpu_family))
)

cpu_flash_dep = declare_dependency(
    sources: files('flash_@0@.c'.format(cpu_family))
)

cpu_ser_dep = declare_dependency(
    sources: files('ser_@0@.c'.format(cpu_family))
)

cpu_i2c_dep = declare_dependency(
    sources: files('i2c_@0@.c'.format(cpu_family))
)
