
EMBED_MOD_DIR := $(USERMOD_DIR)

# Add all C files to SRC_USERMOD.
SRC_USERMOD += $(EMBED_MOD_DIR)/modembed.c

# add module folder to include paths if needed
CFLAGS_USERMOD += -I$(EMBED_MOD_DIR)
