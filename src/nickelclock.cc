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

#include "nc_common.h"
#include "nickelclock.h"

#include <NickelHook.h>

const char nc_qt_property[] = "NickelClock";
const char nc_widget_name[] = "ncLabelWidget";

const char* battery_cap_files[] = {
    "/sys/class/power_supply/battery/capacity",
    "/sys/class/power_supply/mc13892_bat/capacity",
    "/sys/class/power_supply/bd71827_bat/capacity"
};

NC *nc = nullptr;

// This is somewhat arbitrary, but seems a good place to get
// access to the ReadingView after it has been created.
void (*ReadingView__ReaderIsDoneLoading)(ReadingView *_this);
// TimeLabel is what the status bar uses to show the time
TimeLabel *(*TimeLabel__TimeLabel)(TimeLabel *_this, QWidget *parent);

HardwareInterface *(*HardwareFactory__sharedInstance)();
N3BatteryStatusLabel *(*N3BatteryStatusLabel__N3BatteryStatusLabel)(N3BatteryStatusLabel* _this, QWidget *parent);

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
        .name    = "_ZN20N3BatteryStatusLabelC1EP7QWidget",
        .out     = nh_symoutptr(N3BatteryStatusLabel__N3BatteryStatusLabel),
        .desc    = "N3BatteryStatusLabel::N3BatteryStatusLabel()"
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
            QWidget *bl = createBatteryWidget();
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

QWidget* NC::createBatteryWidget()
{
    BatteryType type = settings.batteryType();
    QWidget *battery = new QWidget();
    QHBoxLayout *l = new QHBoxLayout();
    NCBatteryLabel *level = nullptr;
    N3BatteryStatusLabel *icon = nullptr;

    if (type == Level || type == Both) {
        int initLevel = getBatteryLevel();
        level = new NCBatteryLabel(initLevel, settings.batteryLabel());
        level->setStyleSheet(ncLabelStylesheet());
        l->addWidget(level, 0, Qt::AlignVCenter);
    }

    if (type == Icon || type == Both) {
        icon = (N3BatteryStatusLabel*) ::operator new (256); // Actual size 208 bytes
        N3BatteryStatusLabel__N3BatteryStatusLabel(icon, nullptr);
        l->addWidget(icon, 0, Qt::AlignVCenter);
    }

    l->setContentsMargins(0, 0, 0, 0);
    battery->setLayout(l);
    battery->setStyleSheet("padding: 0px; margin: 0px; background-color: transparent;");
    battery->show();
    return battery;
}

// Trying to get the battery level out of Nickel seems to be more trouble
// than it's worth, therefore get it via sysfs
int NC::getBatteryLevel()
{
    int battery = 100;
    if (batteryCapFilename.isEmpty()) {
        for (auto file_name : battery_cap_files) {
            if (QFile::exists(file_name)) {
                batteryCapFilename = file_name;
                break;
            }
        }
    }
    if (batteryCapFilename.isEmpty()) {
        return battery;
    }
    QFile bcFile;
    bcFile.setFileName(batteryCapFilename);
    if (bcFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        bool ok = false;
        battery = bcFile.readAll().trimmed().toInt(&ok);
        if (!ok) {
            nh_log("failed to get battery level");
            battery = 100;
        }
    }
    return battery;
}

NCBatteryLabel::NCBatteryLabel(int initLevel, QString const& lbl, QWidget *parent) 
    : QLabel(parent), label(lbl)
{
    setBatteryLevel(initLevel);
    setObjectName(nc_widget_name);
    set_extra_props(this);
    // No battery_level signal connection here anymore
}

void NCBatteryLabel::setBatteryLevel(int level)
{
    QString txt = label.arg(level);
    setText(txt);
}

void NCBatteryLabel::updateBatteryLevel()
{
    auto *hw = RealHardwareClass::sharedInstance();
    int currentLevel = hw->getBatteryLevel();  // or the correct method for current battery %
    setBatteryLevel(currentLevel);
}


// On recent 4.x firmware versions, the header and footer are setup in 
// Ui_ReadingView::setupUi(). They are ReadingFooter widgets, with names set to 
// "header" and "footer". This makes it easy to find them with findChild().
extern "C" __attribute__((visibility("default"))) void _nc_set_header_clock(ReadingView *_this) 
{
    nc->settings.syncSettings();
    nc->addItemsToFooter(_this);
    if (nc->settings.debugEnabled()) {
        nh_dump_log();
    }
    ReadingView__ReaderIsDoneLoading(_this);
}
