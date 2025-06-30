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

    int getBatteryLevel();  // <--- make it public!

private:
    int origFooterMargin = -1;
    QString origFooterStylesheet;
    QString ncLblStylesheet;
    QRegularExpression footerMarginRe;
    QString batteryCapFilename;
    QRect scrGeom;
    void updateFooterMargins(QLayout *layout);
    void getFooterStylesheet();
    void createNCLabelStylesheet();
    QWidget* createBatteryWidget();
    TimeLabel* createTimeLabel();

    int getBatteryLevelImpl(); // original private version

};


class NCBatteryLabel : public QLabel {
    Q_OBJECT
public:
    NCBatteryLabel(int initLevel, QString const& label, NC* nc, QWidget *parent = nullptr);

    void setBatteryLevel(int level);

private slots:
    void updateBatteryLevel();

private:
    int m_batteryLevel;  // put these two first
    QString m_label;

    NC* m_nc;            // pointer last

    void updateText();
};



#endif
