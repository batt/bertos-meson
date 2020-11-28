eeprom_dep = declare_dependency(
    sources : files('eeprom.c'),
)

eth_dep = declare_dependency(
    dependencies: cpu_eth_dep,
)

i2c_dep = declare_dependency(
    sources : files('i2c.c'),
)

i2c_bitbang_dep = declare_dependency(
    sources : files('i2c_bitbang.c'),
)

flash_dep = declare_dependency(
    dependencies: cpu_flash_dep,
)

timer_dep = declare_dependency(
    sources : files('timer.c'),
    dependencies: cpu_timer_dep,
)

kdebug_dep = declare_dependency(
    sources : files('kdebug.c'),
    dependencies: cpu_kdebug_dep,
) 