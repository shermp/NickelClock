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
const char nc_widget_name[] = "ncTimeLabel";

NC *nc = nullptr;

// This is somewhat arbitrary, but seems a good place to get
// access to the ReadingView after it has been created.
void (*ReadingView__ReaderIsDoneLoading)(ReadingView *_this);
// TimeLabel is what the status bar uses to show the time
TimeLabel *(*TimeLabel__TimeLabel)(TimeLabel *_this, QWidget *parent);

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

NC::NC(QRect const& screenGeom) 
            : QObject(nullptr), 
              settings(screenGeom),
              footerMarginRe("qproperty-footerMargin:\\s*\\d+;")
{
    getFooterStylesheet();
    createTimeLabelStylesheet();
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
void NC::createTimeLabelStylesheet()
{
    if (tlStylesheet.isEmpty()) {
        getFooterStylesheet();
        int index = origFooterStylesheet.indexOf("#caption");
        if (index == -1)
            return;
        tlStylesheet = origFooterStylesheet;
        tlStylesheet.remove(0, index);
        tlStylesheet.replace("#caption", QString("#%1").arg(nc_widget_name));
        tlStylesheet.append(QString("\n#%1 {padding: 0px;}").arg(nc_widget_name));
    }
}

QString const& NC::timeLabelStylesheet()
{
    return tlStylesheet;
}

// The ReadingFooter uses a QHBoxLayout QLayout with a single widget (the 
// "caption"), which is a QLabel.
// We need to add a TimeLabel widget here, and insert some stretchable spacing 
// to ensure that the caption remains centred. 
void NC::addTimeToFooter(ReadingFooter *rf, Position position) 
{
    QLayout *l = nullptr;
    if (rf && !rf->property(nc_qt_property).isValid() && (l = rf->layout())) {
        nh_log("ReadingView header layout found");
        rf->setProperty(nc_qt_property, true);
        QHBoxLayout *hl = qobject_cast<QHBoxLayout*>(l);
        if (hl) {
            nh_log("Adding TimeLabel widget to ReadingView header");
            setFooterStylesheet(rf);
            
            hl->setStretch(0, 2);

            TimeLabel *tl = (TimeLabel*) ::operator new (128); // Actual size 88 bytes
            TimeLabel__TimeLabel(tl, nullptr);
            tl->setObjectName(nc_widget_name);
            auto hAlign = position == Left ? Qt::AlignLeft : Qt::AlignRight;
            tl->setAlignment(hAlign | Qt::AlignVCenter);
            tl->setStyleSheet(timeLabelStylesheet());

            if (position == Left) {
                hl->insertWidget(0, tl, 1, Qt::AlignLeft);
                hl->addStretch(1);
            } else {
                hl->insertStretch(0, 1);
                hl->addWidget(tl, 1, Qt::AlignRight);
            }
        }
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

// On recent 4.x firmware versions, the header and footer are setup in 
// Ui_ReadingView::setupUi(). They are ReadingFooter widgets, with names set to 
// "header" and "footer". This makes it easy to find them with findChild().
extern "C" __attribute__((visibility("default"))) void _nc_set_header_clock(ReadingView *_this) 
{
    nc->settings.syncSettings();
    auto containerName = (nc->settings.clockPlacement() == Header)
                         ? "header" : "footer";

    // Find header or footer
    ReadingFooter *rf = _this->findChild<ReadingFooter*>(containerName);
    if (!rf)
        nh_log("ReadingFooter '%s' not found in ReadingView", containerName);

    nc->addTimeToFooter(rf, nc->settings.clockPosition());
    ReadingView__ReaderIsDoneLoading(_this);
}
