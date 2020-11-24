project('STM32 bootlolader', 'c', default_options: ['b_asneeded=false'])
debug = get_option('debug')

bertos_modules = [
	'cpu',
	'cpu_core',
	'lwip',
	'crc_ccitt',
	'proc',
	'signal',
	'timer',
	'flash',
	'tftp',
]

prj_dir = meson.current_source_dir()


bertos = subproject('bertos', default_options: [
	'modules=@0@'.format(bertos_modules),
	'cpu=STM32F207IG',
	'cpu_frequency=106000000',
	'app_dir=' + prj_dir,
])


bertos_dep = bertos.get_variable('bertos_dep')

debug = true

csrc = [
    'main.c',
    'crc32.c',
    'telnet.c',
	'hw/hw_i2c_bitbang.c',
    'common/heartbeat.c',
    'common/state.c',
    'common/mac.c',
	'common/board_id.c',
	'common/system.c',
	'common/eth_cfg.c',
]

cflags = ['-UARCH',
	'-DARCH=ARCH_BOOT',
	'-Os',
	'-fno-strict-aliasing',
	'-fwrapv',
]

if debug
    cflags += '-D_DEBUG'
endif

main = executable('boot', [csrc],
	name_suffix : 'elf',
	c_args : cflags,
	include_directories : include_directories('.'),
	dependencies: bertos_dep
)


