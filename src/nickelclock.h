#ifndef NICKELCLOCK_H
#define NICKELCLOCK_H

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QRegularExpression>
#include <QString>
#include <QSettings>

typedef QWidget ReadingView;
typedef QWidget ReadingFooter;
typedef QLabel TimeLabel;
typedef QLabel TouchLabel;

enum class TimePos {Left, Right};
enum class TimePlacement {Header, Footer};

#ifndef NICKEL_CLOCK_DIR
    #define NICKEL_CLOCK_DIR "/mnt/onboard/.adds/nickelclock"
#endif

class NCSettings 
{
    public:
        NCSettings(QRect const& screenGeom);
        
        TimePlacement placement();
        TimePos position();
        int hMargin();
    private:
        QSettings settings;
        QString placeKey = "placement";
        QString posKey = "position";
        QString marginKey = "hor_margin";
        QString placeHeader = "header";
        QString placeFooter = "footer";
        QString posLeft = "left";
        QString posRight = "right";
        QString marginAuto = "auto";

        int maxHMargin = 200;

        void syncSettings();
        void setMaxHMargin(QRect const& screenGeom);
};

class NC : public QObject
{
    Q_OBJECT
    public:
        NCSettings settings;
        
        NC(QRect const& screenGeom);
        void addTimeToFooter(ReadingFooter *rf, TimePos position);
        void setFooterStylesheet(ReadingFooter *rf);
        QString const& timeLabelStylesheet();
    private:
        int origFooterMargin = -1;
        QString origFooterStylesheet;
        QString tlStylesheet;
        QRegularExpression footerMarginRe;
        void updateFooterMargins(QLayout *layout);
        void getFooterStylesheet();
        void createTimeLabelStylesheet();
};

#endif
