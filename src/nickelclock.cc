#include <cstddef>
#include <cstdlib>

#include <NickelHook.h>

static struct nh_info NickelClock = {
    .name           = "NickelClock",
    .desc           = "",
    .uninstall_flag = "/mnt/onboard/nc_uninstall",
};

static struct nh_hook NickelClockHook[] = {
    {0},
};

static struct nh_dlsym NickelClockDlsym[] = {
    {0},
};

NickelHook(
    .init  = nullptr,
    .info  = &NickelClock,
    .hook  = NickelClockHook,
    .dlsym = NickelClockDlsym,
)
