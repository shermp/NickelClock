#ifndef NC_BATTERY_H
#define NC_BATTERY_H

#include <QObject>
#include <QLabel>
#include <QFrame>

typedef QObject HardwareInterface;

extern HardwareInterface *(*HardwareFactory__sharedInstance)();
extern uintptr_t** HardwareInterface__vtable;
extern int  (*HardwareInterface__getBatteryLevel)(HardwareInterface* self);
extern uint (*HardwareInterface__chargingState)(HardwareInterface* self);

class NCBatteryLabel : public QFrame
{
    Q_OBJECT

public:
    NCBatteryLabel(bool text_en, bool icon_en, QString const& text_f, QWidget* parent = nullptr);
    ~NCBatteryLabel() override;
    
    QLabel* getLabel() { return text_label; }
    QLabel* getIcon() { return icon; }

public slots:
    void onDarkModeChanged(bool enabled);
    void onBatteryLevel(int level);
    void updateBattery();

private:
    QString batteryIconPathName();
    void setLabels();

    QLabel* text_label;
    QLabel* icon;
    QString text_fmt;

    int battery_level;
    int curr_index;

    bool is_charging;
    bool dark_mode_enabled;
    bool dark_mode_changed;
};

#endif // NC_BATTERY_H