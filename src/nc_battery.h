#ifndef NC_BATTERY_H
#define NC_BATTERY_H

#include <QObject>
#include <QLabel>

typedef QObject PowerTimer;

class NCBatteryLabel : public QLabel
{

private:

    char pw_timer_obj[0x38 * 2];
    PowerTimer* pw_timer = nullptr;

    
};

#endif // NC_BATTERY_H