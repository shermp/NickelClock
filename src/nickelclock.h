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
        void addItemsToFooter(ReadingView *rv);
        void setFooterStylesheet(ReadingFooter *rf);
        QString const& ncLabelStylesheet();
    private:
        int origFooterMargin = -1;
        QString origFooterStylesheet;
        QString ncLblStylesheet;
        QRegularExpression footerMarginRe;
        void updateFooterMargins(QLayout *layout);
        void getFooterStylesheet();
        void createNCLabelStylesheet();
        TimeLabel* createTimeLabel();
};

#endif
