#include "nc_common.h"
#include "nc_settings.h"

NCSettings::NCSettings(QRect const& screenGeom)
    : settings(NICKEL_CLOCK_DIR "/settings.ini", QSettings::IniFormat)
{
    setMaxHMargin(screenGeom);
    syncSettings();
}

void NCSettings::syncSettings()
{
    settings.sync();

    // import legacy settings
    QString lMargin = settings.value("hor_margin").toString();
    QString lPos = settings.value("position").toString();
    QString lPlace = settings.value("placement").toString();

    QString defPos = SL(Right);
    QString defPlace = SL(Header);
    QString defMargin = SL(Auto);
    if (lPos == "left")
        defPos = SL(Left);
    if (lPlace == "footer")
        defPlace == SL(Footer);
    if (lMargin != "auto")
        defMargin = lMargin;
    
    for (auto& k : {QSL("hor_margin"), QSL("position"), QSL("placement")}) {
        settings.remove(k);
    }
    
    for (auto& g : {SL(Clock), SL(Battery)}) {
        settings.beginGroup(g);

        QVariant enabled = settings.value(enabledKey);
        if (enabled.isNull())
            enabled = g == SL(Clock) ? true : false;
        settings.setValue(enabledKey, enabled.toBool());

        QString pos = settings.value(SL(Position), defPos).toString();
        QString place = settings.value(SL(Placement), defPlace).toString();
        if (pos != SL(Left) && pos != SL(Right))
            pos = SL(Right);
        if (place != SL(Header) && place != SL(Footer))
            place = SL(Header);
        settings.setValue(SL(Position), pos);
        settings.setValue(SL(Placement), place);

        if (g == SL(Battery)) {
            QString type = settings.value(SL(BatteryType), SL(Level)).toString();
            if (type != SL(Level) && type != SL(Icon) && type != SL(Both))
                type = SL(Level);
            settings.setValue(SL(BatteryType), type);
        }

        settings.endGroup();
    }
    
    QString marginStr = settings.value(SL(Margin), defMargin).toString();
    if (marginStr != SL(Auto)) {
        bool ok = false;
        int margin = marginStr.toInt(&ok);
        if (!ok || margin > maxHMargin || margin < 0 ) {
            marginStr = SL(Auto);
        } else {
            marginStr = QString::number(margin);
        }
    }
    settings.setValue(SL(Margin), marginStr);

    QString order = settings.value(SL(WidgetOrder), SL(ClockFirst)).toString();
    if (order != SL(ClockFirst) && order != SL(BatteryFirst))
        order = SL(ClockFirst);
    settings.setValue(SL(WidgetOrder), order);

    settings.sync();
}

Position NCSettings::clockPosition() { return position(SL(Clock)); }
Position NCSettings::batteryPosition() { return position(SL(Battery)); }
Placement NCSettings::clockPlacement() { return placement(SL(Clock)); }
Placement NCSettings::batteryPlacement() { return placement(SL(Battery)); }

Position NCSettings::position(QString const& group)
{
    QString key = QStringLiteral("%1/%2").arg(group).arg(SL(Position));
    QString pos = settings.value(key).toString();
    return pos == SL(Left) ? Left : Right;
}

Placement NCSettings::placement(QString const& group)
{
    QString key = QStringLiteral("%1/%2").arg(group).arg(SL(Placement));
    QString place = settings.value(key).toString();
    return place == SL(Header) ? Header : Footer;
}

int NCSettings::margin()
{
    bool ok = false;
    int margin = Auto;
    QString marginStr = settings.value(SL(Margin)).toString();
    if (marginStr != SL(Auto)) {
        margin = marginStr.toInt(&ok);
    }
    return ok ? margin : Auto;
}

void NCSettings::setMaxHMargin(QRect const& screenGeom)
{
    int w = screenGeom.width() < screenGeom.height() ? screenGeom.width()
                                                     : screenGeom.height();
    maxHMargin = w / 4;
}

