include libs/NickelHook/NickelHook.mk

override LIBRARY  := libnickelclock.so
override SOURCES  += src/nickelclock.cc 
override CFLAGS   += -Wall -Wextra -Werror
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers

override PKGCONF  += Qt5Widgets

include libs/NickelHook/NickelHook.mk
