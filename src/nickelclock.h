#ifndef NICKELCLOCK_H
#define NICKELCLOCK_H

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QRegularExpression>
#include <QString>
#include <QFrame>

#include "nc_common.h"
#include "nc_settings.h"

typedef QObject HardwareInterface;
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
        
        NC(QRect const& screenGeom);
        void addItemsToFooter(ReadingView *rv);
        void setFooterStylesheet(ReadingFooter *rf);
        QString const& ncLabelStylesheet();
    private:
        int origFooterMargin = -1;
        QString origFooterStylesheet;
        QString ncLblStylesheet;
        QRegularExpression footerMarginRe;
        QString batteryCapFilename;
        void updateFooterMargins(QLayout *layout);
        void getFooterStylesheet();
        void createNCLabelStylesheet();
        QWidget* createBatteryWidget();
        TimeLabel* createTimeLabel();
        int getBatteryLevel();
};

class NCBatteryLabel : public QLabel
{
    Q_OBJECT
    public:
        NCBatteryLabel(int initLevel, QString const& label, QWidget *parent = nullptr);
    public Q_SLOTS:
        void setBatteryLevel(int level);
    private:
        QString label;
};

#endif
