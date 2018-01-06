#include "notification/hub.h"

std::unordered_map<std::type_index, std::unordered_map<Subscriber*, std::vector<std::function<void()>>>> Hub::g_subscriptions;

void Hub::RemoveSubscriber(Subscriber* subscriber)
{
    for(auto it = g_subscriptions.begin(); it != g_subscriptions.end(); it++)
        (*it).second.erase(subscriber);
}
