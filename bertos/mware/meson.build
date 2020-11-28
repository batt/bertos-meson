event_dep = declare_dependency(
    sources : files('event.c'),
)

hex_dep  = declare_dependency(
    sources : files('hex.c'),
)

sprintf_dep = declare_dependency(
    sources : files('sprintf.c'),
)

formatwr_dep = declare_dependency(
    sources : files('formatwr.c'),
    dependencies : [hex_dep, sprintf_dep],
)


