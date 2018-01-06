#include "notification/subscriber.h"

Subscriber::~Subscriber()
{
    Hub::RemoveSubscriber(this);
}
