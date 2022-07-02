#ifndef NICKELCLOCK_H
#define NICKELCLOCK_H

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QRegularExpression>
#include <QString>

#include "nc_common.h"
#include "nc_settings.h"

typedef QWidget ReadingView;
typedef QWidget ReadingFooter;
typedef QLabel TimeLabel;
typedef QLabel TouchLabel;

class NC : public QObject
{
    Q_OBJECT
    public:
        NCSettings settings;
        
        NC(QRect const& screenGeom);
        void addTimeToFooter(ReadingFooter *rf, Position position);
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
