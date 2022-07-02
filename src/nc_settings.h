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
        Position clockPosition();
        Position batteryPosition();
        Placement clockPlacement();
        Placement batteryPlacement();
        int margin();

    private:
        QSettings settings;
        int maxHMargin = 200;
        QString enabledKey = "Enabled";

        void setMaxHMargin(QRect const& screenGeom);
        Position position(QString const& group);
        Placement placement(QString const& group);
};

#endif // NC_SETTINGS_H
