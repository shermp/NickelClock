#ifndef NC_SETTINGS_H
#define NC_SETTINGS_H

#include <QRect>
#include <QSettings>
#include <QString>

#define QSL(s) QStringLiteral(s)
#define SL(s) QSL(#s)

class NCSettings
{
    public:
        NCSettings(QRect const& screenGeom);

        void syncSettings();

        bool clockEnabled();
        bool batteryEnabled();
        Position clockPosition();
        Position batteryPosition();
        Placement clockPlacement();
        Placement batteryPlacement();
        bool clockInPlacement(Placement const p);
        bool batteryInPlacement(Placement const p);
        BatteryType batteryType();
        QString batteryLabel();
        int margin();
        bool debugEnabled();

    private:
        QSettings settings;
        int maxHMargin = 200;
        QString enabledKey = "Enabled";
        QString batteryLabelKey = "LevelTemplate";
        QString debugKey = "Debug";

        void setMaxHMargin(QRect const& screenGeom);
        Position position(QString const& group);
        Placement placement(QString const& group);
        bool groupEnabled(QString const& group);
};

#endif // NC_SETTINGS_H
