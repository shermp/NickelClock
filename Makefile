include libs/NickelHook/NickelHook.mk

override LIBRARY  := libnickelclock.so
override SOURCES  += src/nickelclock.cc 
override MOCS     += src/nickelclock.h
override CFLAGS   += -Wall -Wextra -Werror
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers

override PKGCONF  += Qt5Widgets

override KOBOROOT += res/uninstall:$(NC_SETTINGS_DIR)/uninstall

ifeq ($(NC_SETTINGS_DIR),)
override NC_SETTINGS_DIR := /mnt/onboard/.adds/nickelclock
endif

override CPPFLAGS += -DNICKEL_CLOCK_DIR='"$(NC_SETTINGS_DIR)"'

include libs/NickelHook/NickelHook.mk
