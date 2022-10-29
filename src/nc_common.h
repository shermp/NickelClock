#ifndef NC_COMMON_H
#define NC_COMMON_H

#ifndef NICKEL_CLOCK_DIR
    #define NICKEL_CLOCK_DIR "/mnt/onboard/.adds/nickelclock"
#endif
  
enum Position { Left, Right };
enum Placement { Header, Footer };
enum Widget { Clock, Battery };
enum BatteryType { Level, Icon, Both };
enum Margin { Auto = -1 };

#endif // NC_COMMON_H
