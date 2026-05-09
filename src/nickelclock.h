#ifndef NICKELCLOCK_H
#define NICKELCLOCK_H

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QPointer>
#include <QRegularExpression>
#include <QString>
#include <QFrame>

#include "nc_common.h"
#include "nc_settings.h"
#include "nc_battery.h"

typedef QObject HardwareInterface;
typedef QObject Device;
typedef QObject SelectionController;
typedef QWidget ReadingView;
typedef QWidget ReadingFooter;
typedef QLabel TimeLabel;
typedef QLabel TouchLabel;
typedef QLabel N3BatteryStatusLabel;

class NC : public QObject
{
    Q_OBJECT
    public:
        NCSettings settings;
        QPointer<QWidget> reading_view;

        NC(QRect const& screenGeom);
        void setReadingView(ReadingView *rv);
        void addItemsToFooter(ReadingView *rv);
        void setFooterStylesheet(ReadingFooter *rf);
        QString const& ncLabelStylesheet();

    public slots:
        void onDarkModeChanged();
        void onFooterMenuClosed();

    signals:
        void darkModeChanged(bool enabled);

    private:
        int origFooterMargin = -1;
        QString origFooterStylesheet;
        QString ncLblStylesheet;
        QRegularExpression footerMarginRe;
        QString batteryCapFilename;
        QRect scrGeom;
        ReadingView *readingView;
        void updateFooterMargins(QLayout *layout);
        void getFooterStylesheet();
        void createNCLabelStylesheet();
        NCBatteryLabel* createBatteryWidget();
        TimeLabel* createTimeLabel();
};

#endif
