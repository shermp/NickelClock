#include <cstddef>
#include <cstdlib>

#include <Qt>
#include <QGuiApplication>
#include <QWidget>
#include <QHBoxLayout>
#include <QFile>
#include <QLabel>
#include <QVariant>
#include <QSettings>
#include <QMargins>
#include <QScreen>
#include <QMetaEnum>

#include "nc_common.h"
#include "nickelclock.h"

#include <NickelHook.h>

const char nc_qt_property[] = "NickelClock";
const char nc_widget_name[] = "ncLabelWidget";

NC *nc = nullptr;
static bool has_color_display = false;

// This is somewhat arbitrary, but seems a good place to get
// access to the ReadingView after it has been created.
void (*ReadingView__ReaderIsDoneLoading)(ReadingView *_this);

// ReadingView provides a DarkModeChanged signal, but that does not provide the
// actual value, so we need to call this method as well
bool (*ReadingView__canEnableDarkMode)(ReadingView *_this);

// TimeLabel is what the status bar uses to show the time
TimeLabel *(*TimeLabel__TimeLabel)(TimeLabel *_this, QWidget *parent);

Device *(*Device__getCurrentDevice)();
bool (*Device__hasColorDisplay)(Device *_this);

static struct nh_info NickelClock = {
    .name           = "NickelClock",
    .desc           = "Set an always displayed clock when reading",
    .uninstall_flag = nullptr,
    .uninstall_xflag = NICKEL_CLOCK_DIR "/uninstall",
    .failsafe_delay = 10
};

static struct nh_hook NickelClockHook[] = {
    {
        .sym     = "_ZN11ReadingView19readerIsDoneLoadingEv", 
        .sym_new = "_nc_set_header_clock",
        .lib     = "libnickel.so.1.0.0",
        .out     = nh_symoutptr(ReadingView__ReaderIsDoneLoading),
        .desc    = "footer progress update"
    },
    {0},
};

static struct nh_dlsym NickelClockDlsym[] = {
    {
        .name    = "_ZN9TimeLabelC1EP7QWidget",
        .out     = nh_symoutptr(TimeLabel__TimeLabel),
        .desc    = "TimeLabel::TimeLabel()"
    },
    {
        .name    = "_ZN15HardwareFactory14sharedInstanceEv",
        .out     = nh_symoutptr(HardwareFactory__sharedInstance),
        .desc    = "HardwareFactory::sharedInstance()"
    },
    {
        .name    = "_ZTV17HardwareInterface",
        .out     = nh_symoutptr(HardwareInterface__vtable),
        .desc    = "HardwareInterface::vtable"
    },
    {   .name    = "_ZNK17HardwareInterface15getBatteryLevelEv",
        .out     = nh_symoutptr(HardwareInterface__getBatteryLevel),
        .desc    = "HardwareInterface::getBatteryLevel()",
    },
    {
        .name    = "_ZN17HardwareInterface13chargingStateEv",
        .out     = nh_symoutptr(HardwareInterface__chargingState),
        .desc    = "HardwareInterface::chargingState()",
    },
    {
        .name    = "_ZN11ReadingView17canEnableDarkModeEv",
        .out     = nh_symoutptr(ReadingView__canEnableDarkMode),
        .desc    = "ReadingView::canEnableDarkMode()",
        .optional= true
    },
    {
        .name    = "_ZNK6Device15hasColorDisplayEv",
        .out     = nh_symoutptr(Device__hasColorDisplay),
        .desc    = "Device::hasColorDisplay()",
        .optional= true,
    },
    {
        .name    = "_ZN6Device16getCurrentDeviceEv",
        .out     = nh_symoutptr(Device__getCurrentDevice),
        .desc    = "Device::getCurrentDevice()",
        .optional= true,
    },
    {0},
};

static int nc_init()
{
    QScreen *scr = QGuiApplication::primaryScreen();
    QRect const geom = scr->geometry();
    nc = new NC(geom);
    if (!nc)
        return 1;

    if(Device__hasColorDisplay && Device__getCurrentDevice) {
        has_color_display = Device__hasColorDisplay(Device__getCurrentDevice());
    }

    return 0;
}

static bool nc_uninstall()
{
    nh_delete_file(NICKEL_CLOCK_DIR "/settings.ini");
    nh_delete_dir(NICKEL_CLOCK_DIR);
    return true;
}

NickelHook(
    .init  = &nc_init,
    .info  = &NickelClock,
    .hook  = NickelClockHook,
    .dlsym = NickelClockDlsym,
    .uninstall = &nc_uninstall
)

// Older firmware versions have [newHeader=true] and [newFooter=true] as 
// part of their QSS selector. Create and set those properties here.
static void set_extra_props(QWidget* w) {
    if (w) {
        for (auto prop : {"newHeader", "newFooter"}) {
            w->setProperty(prop, true);
        }
    }
}

NC::NC(QRect const& screenGeom) 
            : QObject(nullptr), 
              settings(screenGeom),
              footerMarginRe("qproperty-footerMargin:\\s*\\d+;"),
              scrGeom(screenGeom)
{
    getFooterStylesheet();
    createNCLabelStylesheet();
}

void NC::setReadingView(ReadingView *rv)
{
    readingView = rv;
}

void NC::getFooterStylesheet()
{
    if (origFooterStylesheet.isEmpty()) {
        QFile rfStyleFile(":/qss/ReadingFooter.qss");
        if (rfStyleFile.open(QIODevice::ReadOnly)) {
            origFooterStylesheet = rfStyleFile.readAll();
        }
    }
}

// Creates a stylesheet for our TimeLabel which is derived from the
// ReadingFooter stylesheet, without the ReadingFooter selectors
void NC::createNCLabelStylesheet()
{
    if (ncLblStylesheet.isEmpty()) {
        getFooterStylesheet();
        int index = origFooterStylesheet.indexOf("#caption");
        if (index == -1)
            return;
        ncLblStylesheet = origFooterStylesheet;
        ncLblStylesheet.remove(0, index);
        ncLblStylesheet.replace("#caption", QString("#%1").arg(nc_widget_name));
        ncLblStylesheet.append(QString("\n#%1 {padding: 0px;}").arg(nc_widget_name));
    }
}

QString const& NC::ncLabelStylesheet()
{
    return ncLblStylesheet;
}

// The ReadingFooter uses a QHBoxLayout QLayout with a single widget (the 
// "caption"), which is a QLabel.
// We need to add a TimeLabel widget here, and insert some stretchable spacing 
// to ensure that the caption remains centred. 
void NC::addItemsToFooter(ReadingView *rv) 
{
    for (auto p : {Header, Footer}) {
        const char *fName = p == Header ? "header" : "footer";
        ReadingFooter *rf = rv->findChild<ReadingFooter*>(fName);
        if (!rf) {
            nh_log("could not find %s", fName);
            continue;
        }
        if (rf->property(nc_qt_property).isValid()) {
            nh_log("skipping already setup %s", fName);
            continue;
        }
        QHBoxLayout *layout = qobject_cast<QHBoxLayout*>(rf->layout());
        if (!layout) {
            nh_log("could not obtain QHBoxLayout from %s", fName);
            continue;
        }
        if (!settings.clockInPlacement(p) && !settings.batteryInPlacement(p)) {
            nh_log("nothing to add to %s", fName);
            continue;
        }
        // Set the stretch value of the existing caption
        layout->setStretch(0, 2);
        setFooterStylesheet(rf);
        // Add some spacing between widgets (as a percentage of screen width)
        auto spacing = static_cast<int>(std::round(scrGeom.width() * 0.015f));
        layout->setSpacing(spacing);
        // Both clock & battery in the same postion and placement is not allowed
        if (settings.clockInPlacement(p) && settings.batteryInPlacement(p) 
            && settings.clockPosition() == settings.batteryPosition()) {
                nh_log("clock and battery level cannot share the same placement and position");
                continue;
        }
        
        bool lw = false;
        bool rw = false;
        if (settings.clockInPlacement(p)) {
            TimeLabel *tl = createTimeLabel();
            if (settings.clockPosition() == Left) {
                layout->insertWidget(0, tl, 1, Qt::AlignLeft);
                lw = true;
            } else {
                layout->addWidget(tl, 1, Qt::AlignRight);
                rw = true;
            }
        }
        if (settings.batteryInPlacement(p)) {
            NCBatteryLabel *bl = createBatteryWidget();
            if (settings.batteryPosition() == Left) {
                layout->insertWidget(0, bl, 1, Qt::AlignLeft);
                lw = true;
            } else {
                layout->addWidget(bl, 1, Qt::AlignRight);
                rw = true;
            }
        }
        if (!lw)
            layout->insertStretch(0, 1);
        if (!rw)
            layout->addStretch(1);

        rf->setProperty(nc_qt_property, true);
    }
}

// Nickel sometimes polishes the ReadingFooter widget, which overrides settable 
// values back to their stylesheet default Therefore replace the ReadingFooter 
// stylesheet with customized margins instead.
void NC::setFooterStylesheet(ReadingFooter *rf)
{
    if (!rf || !rf->layout())
        return;
    auto l = rf->layout();
    if (origFooterMargin < 0)
        origFooterMargin = l->contentsMargins().left();
    int newMargin = settings.margin();
    if (newMargin < 0)
        newMargin = origFooterMargin / 10;
    QString s = QStringLiteral("qproperty-footerMargin: %1;").arg(newMargin);
    QString ss = origFooterStylesheet;
    rf->setStyleSheet(ss.replace(footerMarginRe, s));
}

void NC::onDarkModeChanged()
{
    if (ReadingView__canEnableDarkMode) {
        emit darkModeChanged(ReadingView__canEnableDarkMode(readingView));
    }
}

TimeLabel* NC::createTimeLabel()
{
    TimeLabel *tl = (TimeLabel*) ::operator new (128); // Actual size 88 bytes
    TimeLabel__TimeLabel(tl, nullptr);
    tl->setObjectName(nc_widget_name);
    auto hAlign = settings.clockPosition() == Left ? Qt::AlignLeft : Qt::AlignRight;
    tl->setAlignment(hAlign | Qt::AlignVCenter);
    set_extra_props(tl);
    tl->setStyleSheet(ncLabelStylesheet());
    return tl;
}

NCBatteryLabel* NC::createBatteryWidget()
{
    BatteryType type = settings.batteryType();
    QString level_fmt = settings.batteryLabel();
    bool level_enabled = (type == Level || type == Both);
    bool icon_enabled = (type == Icon || type == Both);
    NCBatteryLabel *battery = new NCBatteryLabel(level_enabled, icon_enabled, level_fmt);

    auto hAlign = settings.batteryPosition() == Left ? Qt::AlignLeft : Qt::AlignRight;
    for (auto l : {battery->getLabel(), battery->getIcon()}) {
        if (l) {
            l->setObjectName(nc_widget_name);
            l->setAlignment(hAlign | Qt::AlignVCenter);
            l->setStyleSheet(ncLabelStylesheet());
            set_extra_props(l);
        }
    }
    battery->setObjectName(nc_widget_name);
    battery->setStyleSheet(ncLabelStylesheet());
    set_extra_props(battery);
    QObject::connect(this, &NC::darkModeChanged, battery, &NCBatteryLabel::onDarkModeChanged, Qt::UniqueConnection);
    return battery;
}

// On colour Kobos, SelectionController::onInlineDefinitionResults adds two
// extra attrs (the B&W Kobos have 4, colour have 6) to the ReadingView when the
// dictionary popup appears, which enable "full" refreshes (the ones that flash
// the elements before redrawing) and never clears them. Because it never clears
// them, our label that refreshes every minute also flashes every minute, which
// is distracting.
//
// We hook selectionModeOff and fix this, but the attrs are custom in Kobo's Qt
// so we need to jump through some hoops to resolve them. Don't just use
// hardcoded ints as Qt adds more attrs over time and Kobo's extra ones might
// shift (or not even exist on older firmware)
static const char* const extraAttrs[] = {
    "WA_KoboEpdUpdateModeFull",
    "WA_KoboEpdWfModeGCC16",
};

// QObject::staticQtMetaObject is protected; re-expose it via a derived class
// so we can look up Qt namespace enums by name on older Qt (pre-Q_NAMESPACE).
namespace {
struct QtMetaAccess : QObject {
    using QObject::staticQtMetaObject;
};
}

static const QVector<Qt::WidgetAttribute>& resolvedExtraAttrs()
{
    static const QVector<Qt::WidgetAttribute> v = [] {
        QVector<Qt::WidgetAttribute> r;

        if(!has_color_display) {
            nh_log("No color display, not fixing SelectionController");
            return r;
        }

        const QMetaObject &mo = QtMetaAccess::staticQtMetaObject;
        int enumIdx = mo.indexOfEnumerator("WidgetAttribute");
        if (enumIdx < 0) {
            nh_log("could not find Qt::WidgetAttribute meta enum");
            return r;
        }
        QMetaEnum me = mo.enumerator(enumIdx);
        for (auto& name : extraAttrs) {
            bool ok = false;
            int value = me.keyToValue(name, &ok);
            if (ok) {
                r.push_back(static_cast<Qt::WidgetAttribute>(value));
                // nh_log("mapped Qt::WidgetAttribute::%s -> 0x%02X", name, value);
            } else {
                nh_log("unknown Qt::WidgetAttribute key: %s", name);
            }
        }
        return r;
    }();
    return v;
}

void NC::onFooterMenuClosed()
{
    if(reading_view) {
        for(auto attr : resolvedExtraAttrs()) {
            reading_view->setAttribute(attr, false);
        }
    }
}

// On recent 4.x firmware versions, the header and footer are setup in 
// Ui_ReadingView::setupUi(). They are ReadingFooter widgets, with names set to 
// "header" and "footer". This makes it easy to find them with findChild().
extern "C" __attribute__((visibility("default"))) void _nc_set_header_clock(ReadingView *_this) 
{
    nc->settings.syncSettings();
    nc->setReadingView(_this);
    nc->addItemsToFooter(_this);
    if (!QObject::connect(_this, SIGNAL(darkModeChangedSignal()), nc, SLOT(onDarkModeChanged()), Qt::UniqueConnection)) {
        nh_log("Connect to ReadingView::darkModeChangedSignal() failed");
    }
    nc->reading_view = _this;

    SelectionController* sc = nullptr;
    auto children = _this->findChildren<QObject*>(QString(), Qt::FindDirectChildrenOnly);
    for (auto child : children) {
        if (QLatin1Literal("SelectionController") == child->metaObject()->className()) {
            sc = child;
            break;
        }
    }
    if (sc) {
        if (!QObject::connect(sc, SIGNAL(closeFooterMenu()), nc, SLOT(onFooterMenuClosed()), Qt::UniqueConnection)) {
            nh_log("Connect to SelectionController::closeFooterMenu() failed");
        }
    } else {
        nh_log("%s", "SelectionController not found");
    }

    if (nc->settings.debugEnabled()) {
        nh_dump_log();
    }
    ReadingView__ReaderIsDoneLoading(_this);
}
