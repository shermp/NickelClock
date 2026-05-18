#include <cmath>

#include <QPaintEvent>
#include <QHBoxLayout>
#include <QImage>
#include <QBitmap>

#include "nc_battery.h"

HardwareInterface *(*HardwareFactory__sharedInstance)() = nullptr;

uintptr_t** HardwareInterface__vtable = nullptr;
int  (*HardwareInterface__getBatteryLevel)(HardwareInterface* self) = nullptr;
uint (*HardwareInterface__chargingState)(HardwareInterface* self) = nullptr;

// I took this from my PR for kobo-tweaks. See:
// https://github.com/redphx/kobo-tweaks/pull/11
template <typename F>
static F get_derived_hw_interface_method(F HWInterfaceFunc, HardwareInterface* hw)
{
    struct VPtr {
        uintptr_t** v;
    };
    if (!HWInterfaceFunc) {
        return nullptr;
    }
    // The list of method pointers starts from the third entry
    // in the vtable (this is what the class vptr variable points to)
    uintptr_t** hwiVtr = HardwareInterface__vtable + 2;
    
    // Search the HardwareInterface vtable for the offset to the method 
    // we want to call on the derived object. 
    // Iterate at least 8 times, because the vtable has a null pointer
    // at the destructor offset (4).
    for (int offset = 0; offset < 8 || hwiVtr[offset] != nullptr; ++offset) {
        uintptr_t* f = hwiVtr[offset];

        // Method found at this offset
        // Return the method at this offset in the derived vtable
        if (f == reinterpret_cast<uintptr_t*>(HWInterfaceFunc)) {
            auto derivedVptr = reinterpret_cast<VPtr*>(hw);
            return reinterpret_cast<F>(derivedVptr->v[offset]);
        }
    }
    return nullptr;
}

// Choose from one of the 20 battery level icons in the firmware based
// on the provided battery level. Uses the same algorithm as used by
// GenericBatteryStatusLabel::getBatteryPixmapIndex
static int get_battery_level_index(int level)
{
    double l = static_cast<double>(level);
    if (l > 100.0f) {
        l = 100.0f;
    } else if (l < 1.0f) {
        l = 1.0f;
    }

    double index = std::ceil((l / 100.00f) * 20.00f); 
    return static_cast<int>(index);
}

NCBatteryLabel::NCBatteryLabel(bool text_en, bool icon_en, QString const& text_f, QWidget *parent)
    : QFrame(parent),
      hwi(nullptr),
      text_label(nullptr),
      icon(nullptr),
      text_fmt(text_f),
      battery_level(0),
      is_charging(false),
      dark_mode_enabled(false),
      dark_mode_changed(false)
{
    QHBoxLayout* layout = new QHBoxLayout();
    if (text_en) {
        text_label = new QLabel();
        layout->addWidget(text_label);
    }
    if (icon_en) {
        icon = new QLabel();
        layout->addWidget(icon);
    }
    layout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(layout);
    this->setStyleSheet("padding: 0px; margin: 0px; background-color: transparent;");

    hwi = HardwareFactory__sharedInstance();
    battery_level_fn = get_derived_hw_interface_method<decltype(battery_level_fn)>(HardwareInterface__getBatteryLevel, hwi);
    charging_state_fn = get_derived_hw_interface_method<decltype(charging_state_fn)>(HardwareInterface__chargingState, hwi);

    QObject::connect(hwi, SIGNAL(battery_level(int)), this, SLOT(onBatteryLevel(int)));

    QObject::connect(hwi, SIGNAL(battery_changed()), this, SLOT(updateBattery()));

    QObject::connect(hwi, SIGNAL(usb_plugged()), this, SLOT(updateBattery()));
    QObject::connect(hwi, SIGNAL(usb_ac_plugged()), this, SLOT(updateBattery()));

    QObject::connect(hwi, SIGNAL(usb_unplugged()), this, SLOT(updateBattery()));
    QObject::connect(hwi, SIGNAL(usb_ac_unplugged()), this, SLOT(updateBattery()));

    updateBattery();
}

NCBatteryLabel::~NCBatteryLabel()
{
    QObject::disconnect(hwi, SIGNAL(battery_level(int)), this, SLOT(onBatteryLevel(int)));

    QObject::disconnect(hwi, SIGNAL(battery_changed()), this, SLOT(updateBattery()));

    QObject::disconnect(hwi, SIGNAL(usb_plugged()), this, SLOT(updateBattery()));
    QObject::disconnect(hwi, SIGNAL(usb_ac_plugged()), this, SLOT(updateBattery()));

    QObject::disconnect(hwi, SIGNAL(usb_unplugged()), this, SLOT(updateBattery()));
    QObject::disconnect(hwi, SIGNAL(usb_ac_unplugged()), this, SLOT(updateBattery()));
}


void NCBatteryLabel::onBatteryLevel(int level)
{
    (void)level;
    updateBattery();
}

void NCBatteryLabel::updateBattery()
{
    int bl = battery_level_fn(hwi);
    bool charging = static_cast<bool>(charging_state_fn(hwi));

    if (bl != battery_level || charging != is_charging || dark_mode_changed) {
        battery_level = bl;
        is_charging = charging;
        setLabels();
    }
}

void NCBatteryLabel::onDarkModeChanged(bool enabled)
{
    dark_mode_enabled = enabled;
    dark_mode_changed = true;
    updateBattery();
    dark_mode_changed = false;
}

QString NCBatteryLabel::batteryIconPathName()
{
    int index = get_battery_level_index(battery_level);
    if (is_charging) {
        return QStringLiteral(":/images/statusbar/battery_charging_%1.png").arg(index, 2, 10, QLatin1Char('0'));
    } else {
        return QStringLiteral(":/images/statusbar/battery_%1.png").arg(index, 2, 10, QLatin1Char('0'));
    }
    return QString();
}

void NCBatteryLabel::setLabels()
{
    if (text_label) {
        text_label->setText(text_fmt.arg(battery_level));
    }
    if (icon) {
        QString icon_path = batteryIconPathName();
        QString icon_key = dark_mode_enabled ? icon_path + ".inverted" : icon_path;

        if (!icon_cache.contains(icon_key)) {
            QImage icon_png(icon_path);
            if (dark_mode_enabled) {
                icon_png.invertPixels(QImage::InvertRgb);
            }
            // The battery icons have quite a large border, so crop to opaque area
            QPixmap icon_px = QPixmap::fromImage(icon_png);
            auto bb = QRegion(icon_px.mask()).boundingRect();
            icon_cache.insert(icon_key, icon_px.copy(bb));
        }
        icon->setPixmap(icon_cache.value(icon_key));
    }
}
