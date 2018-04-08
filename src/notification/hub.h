/*
    Ivo - a free software for unfolding 3D models and papercrafting
    Copyright (C) 2015-2018 Oleksii Sierov (seriousalexej@gmail.com)
	
    Ivo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ivo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Ivo.  If not, see <http://www.gnu.org/licenses/>.
*/
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
