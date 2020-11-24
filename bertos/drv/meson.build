flash_dep = declare_dependency(
    dependencies: [cpu_flash_dep, kblock_dep]
)

timer_dep = declare_dependency(
    sources : files('timer.c'),
    dependencies: cpu_timer_dep,
)

kdebug_dep = declare_dependency(
    sources : files('kdebug.c'),
    dependencies: cpu_kdebug_dep,
) 