kblock_dep = declare_dependency(
    sources : files('kblock.c'),
)

kfile_dep = declare_dependency(
    sources : files('kfile.c'),
)

kfile_block_dep = declare_dependency(
    sources : files('kfile_block.c'),
    dependencies : kfile_dep,
)


kblock_ram_dep = declare_dependency(
    sources : files('kblock_ram.c'),
    dependencies : kblock_dep 
)
