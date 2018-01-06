#ifndef IVO_NOTIFICATION_H
#define IVO_NOTIFICATION_H

#define NOTIFICATION(notif)\
    struct notif {}

#define NOTIFY(notif)\
    Hub::Notify<notif>()

#endif
