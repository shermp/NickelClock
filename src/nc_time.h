#ifndef NC_TIME_H
#define NC_TIME_H

#include <QObject>
#include <QLabel>
#include <QString>
#include <QTime>
#include <QEvent>

typedef QObject PowerTimer;

extern void (*PowerTimer__PowerTimer)(PowerTimer* _this, QString  const& name, QObject* parent);
extern void (*PowerTimer__PowerTimer_Destructor)(PowerTimer* _this);

extern void (*PowerTimer__fireIn)(PowerTimer* _this, int time_ms);

class NCTimeLabel : public QLabel
{
    Q_OBJECT
public:
    NCTimeLabel(bool onlyUpdateOnParent, QWidget *parent = nullptr);
    ~NCTimeLabel() override;

    void setEvFilterObj(QObject* obj);

public slots:
    void setTime();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    PowerTimer* pw_timer = nullptr;

    QTime curr_time;
    bool only_update_on_parent;
    bool paint_enabled;
    QObject* ev_filter_obj;
};

#endif // NC_TIME_H