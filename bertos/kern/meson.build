proc_dep = declare_dependency(
    sources : files('proc.c'),
    dependencies: cpu_kern_dep,
)

signal_dep = declare_dependency(
    sources : files('signal.c'),
    dependencies: proc_dep,
)