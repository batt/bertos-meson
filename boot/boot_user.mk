#
# User makefile.
# Edit this file to change compiler options and related stuff.
#

# Programmer interface configuration, see http://dev.bertos.org/wiki/ProgrammerInterface for help
boot_PROGRAMMER_TYPE = none
boot_PROGRAMMER_PORT = none

# Files included by the user.
boot_USER_CSRC = \
    $(boot_SRC_PATH)/main.c \
    $(boot_SRC_PATH)/crc32.c \
    $(boot_SRC_PATH)/telnet.c \
    common/heartbeat.c \
    common/state.c \
    common/mac.c \
	common/board_id.c \
	common/system.c \
	common/eth_cfg.c \
    bertos/verstag.c \
    #

# Files included by the user.
boot_USER_PCSRC = \
	#

# Files included by the user.
boot_USER_CPPASRC = \
	#

# Files included by the user.
boot_USER_CXXSRC = \
	#

# Files included by the user.
boot_USER_ASRC = \
	#

# Flags included by the user.
boot_USER_LDFLAGS = \
	#

# Flags included by the user.
boot_USER_CPPAFLAGS = \
	#

# Flags included by the user.
boot_USER_CPPFLAGS = \
	-UARCH \
	-D'ARCH=(ARCH_BOOT)' \
	-Os \
	-fno-strict-aliasing \
	-fwrapv \
	#
