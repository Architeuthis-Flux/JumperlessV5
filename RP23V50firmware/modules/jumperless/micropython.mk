JUMPERLESS_MOD_DIR := $(USERMOD_DIR)

# Add the Jumperless module C file to SRC_USERMOD_C for QSTR processing
SRC_USERMOD_C += $(JUMPERLESS_MOD_DIR)/modjumperless.c
SRC_USERMOD_C += $(JUMPERLESS_MOD_DIR)/module_stubs.c

# Add the module directory to include paths
CFLAGS_USERMOD += -I$(JUMPERLESS_MOD_DIR) 


# OOFATFS_DIR = lib/oofatfs

# # this sets the config file for FatFs
# CFLAGS_THIRDPARTY += -DFFCONF_H=\"$(OOFATFS_DIR)/ffconf.h\"

# ifeq ($(MICROPY_VFS_FAT),1)
# CFLAGS_EXTMOD += -DMICROPY_VFS_FAT=1
# SRC_THIRDPARTY_C += $(addprefix $(OOFATFS_DIR)/,\
# 	ff.c \
# 	ffunicode.c \
# 	)
# endif