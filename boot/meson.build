project('STM32 bootlolader', 'c', default_options: ['optimization=s', 'debug=true', 'b_pie=false', 'b_staticpic=false', 'b_asneeded=false'])

bertos_modules = [
	'cpu_core',
	'eeprom',
	'eth',
	'event',
	'i2c',
	'i2c_bitbang',
	'kblock',
	'kfile_block',
	'lwip',
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
	'-fno-strict-aliasing',
	'-fwrapv',
	'-ffunction-sections',
	'-fdata-sections',
	'-mcpu=cortex-m3', 
	'-ggdb',
	'-g3', '-gdwarf-2',
	'-mthumb', '-mno-thumb-interwork',
    '-fwrapv', '-fverbose-asm',

	'-W', '-Wformat', '-Wall', '-Wundef', '-Wpointer-arith', '-Wcast-qual',
	'-Wcast-align', '-Wwrite-strings', '-Wsign-compare',
	'-Wmissing-noreturn',
	'-Wextra', '-Wstrict-aliasing=2',

	'-Wmissing-prototypes', '-Wstrict-prototypes',
	'-Wno-cast-function-type',

	'-std=gnu99'
]

boot = executable('boot', [csrc],
	name_suffix : 'elf',
	c_args : cflags,
	link_args: '-Wl,-Map=boot.map,--cref', 
	include_directories : include_directories('.'),
	dependencies: bertos_dep
)

objcopy = find_program('objcopy')
boot_bin = custom_target('boot.bin', 
	output: 'boot.bin',
	build_by_default: true,
	command: [objcopy, '-O', 'binary', 'boot.elf', '@OUTPUT@'],
	depends: boot,
)

boot_hex = custom_target('boot.hex', 
	output: 'boot.hex',
	build_by_default: true,
	command: [objcopy, '-O', 'ihex', 'boot.elf', '@OUTPUT@'],
	depends: boot,
)