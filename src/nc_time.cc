#include <QPaintEvent>
#include <NickelHook.h>
#include "nc_time.h"

void (*PowerTimer__PowerTimer)(PowerTimer* _this, QString const& name, QObject* parent) = nullptr;
void (*PowerTimer__PowerTimer_Destructor)(PowerTimer* _this) = nullptr;
void (*PowerTimer__fireIn)(PowerTimer* _this, int time_ms) = nullptr;

NCTimeLabel::NCTimeLabel(bool onlyUpdateOnParent, QWidget *parent) : QLabel(parent), 
                                                                   only_update_on_parent(onlyUpdateOnParent),
                                                                   paint_enabled(false),
                                                                   ev_filter_obj(nullptr)
{
    pw_timer = reinterpret_cast<PowerTimer*>(pw_timer_obj);

    if (PowerTimer__PowerTimer) {
        PowerTimer__PowerTimer(pw_timer, QStringLiteral("nc_time_label"), nullptr);
    }
    QObject::connect(pw_timer, SIGNAL(timeout()), this, SLOT(setTime()));
    setTime();
}

NCTimeLabel::~NCTimeLabel()
{
    QObject::disconnect(pw_timer, SIGNAL(timeout()), this, SLOT(setTime()));
    if (PowerTimer__PowerTimer_Destructor) {
        PowerTimer__PowerTimer_Destructor(pw_timer);
    }

    pw_timer = {};
}

void NCTimeLabel::setEvFilterObj(QObject *obj)
{
    ev_filter_obj = obj;
}

bool NCTimeLabel::eventFilter(QObject* obj, QEvent *event)
{
    if (only_update_on_parent) {
        if (obj == ev_filter_obj && event->type() == QEvent::Paint) {
            QPaintEvent* pe = static_cast<QPaintEvent*>(event);
            if (pe->rect().width() != contentsRect().width() &&
                pe->rect().height() != contentsRect().height()) {
                nh_log("QPaintEvent W: %d H: %d", pe->rect().width(), pe->rect().height());
                nh_log("NCTimeLabel W: %d H: %d", contentsRect().width(), contentsRect().height());
                setText(curr_time.toString("h:mm ap"));
            }
        }
    }
    return false;
}

void NCTimeLabel::setTime()
{
    curr_time = QTime::currentTime();
    int sec = curr_time.second();
    int sec_to_next_min = 59 - sec;
    
    PowerTimer__fireIn(pw_timer, (sec_to_next_min * 1000) + 500);

    if (!only_update_on_parent) {
        setText(curr_time.toString("h:mm ap"));
    }
}