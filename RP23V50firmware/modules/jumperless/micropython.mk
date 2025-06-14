JUMPERLESS_MOD_DIR := $(USERMOD_DIR)

# Add the Jumperless module C file to SRC_USERMOD_C for QSTR processing
SRC_USERMOD_C += $(JUMPERLESS_MOD_DIR)/modjumperless.c

# Add the module directory to include paths
CFLAGS_USERMOD += -I$(JUMPERLESS_MOD_DIR) 