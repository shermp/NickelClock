include libs/NickelHook/NickelHook.mk

override LIBRARY  := libnickelclock.so
override SOURCES  += src/nickelclock.cc 
override CFLAGS   += -Wall -Wextra -Werror
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers

include libs/NickelHook/NickelHook.mk
