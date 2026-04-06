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
    NCBatteryLabel(bool text_en, bool icon_en, QString const& text_f, bool onlyUpdateOnParent, QWidget* parent = nullptr);
    ~NCBatteryLabel() override;

    void setEvFilterObj(QObject* obj);
    
    QLabel* getLabel() { return text_label; }
    QLabel* getIcon() { return icon; }

    void onDarkModeChanged(bool enabled);

public slots:
    void onBatteryLevel(int level);
    void onUsbPlugged();
    void onUsbUnplugged();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QString batteryIconPathName();
    void setLabels();

    QLabel* text_label;
    QLabel* icon;
    QString text_fmt;

    int battery_level;
    int curr_index;

    bool is_charging;

    bool only_update_on_parent;
    QObject* ev_filter_obj;

    bool dark_mode_enabled;
};

#endif // NC_BATTERY_H