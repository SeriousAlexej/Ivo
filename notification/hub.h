#ifndef IVO_HUB_H
#define IVO_HUB_H
#include <iostream>
#include <type_traits>
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <vector>
#include "notification/notification.h"

class Subscriber;

class Hub
{
public:
    Hub() = delete;
    Hub(Hub&) = delete;

    template<typename TNotification>
    static void Notify()
    {
        TReactions& reactions = g_subscriptions[std::type_index(typeid(typename std::decay<TNotification>::type))];
        for(auto it = reactions.begin(); it != reactions.end(); it++)
            for(TCallback& t : (*it).second)
                t();
    }

private:
    friend class Subscriber;

    using TCallback = std::function<void()>;
    using TCallbacks = std::vector<TCallback>;
    using TReactions = std::unordered_map<Subscriber*, TCallbacks>;

    template<typename TNotification>
    static void AddSubscriber(Subscriber* subscriber, std::function<void()>&& func)
    {
        TReactions& reactions = g_subscriptions[std::type_index(typeid(typename std::decay<TNotification>::type))];
        TCallbacks& callbacks = reactions[subscriber];
        callbacks.emplace_back(std::move(func));
    }

    static void RemoveSubscriber(Subscriber* subscriber);

    static std::unordered_map<std::type_index, TReactions> g_subscriptions;
};

#endif
