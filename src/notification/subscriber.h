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
#ifndef IVO_SUBSCRIBER_H
#define IVO_SUBSCRIBER_H
#include "notification/hub.h"

class Subscriber
{
public:
    Subscriber() = default;
    virtual ~Subscriber();

protected:
    template<typename TNotification,
             typename TMethod,
             typename std::enable_if<std::is_member_function_pointer<TMethod>::value, int>::type = 0>
    void Subscribe(TMethod&& callback)
    {
        using TClass = typename std::remove_pointer<typename decltype(std::mem_fn(callback))::argument_type>::type;
        static_assert(std::is_base_of<Subscriber, TClass>::value, "");
        Hub::AddSubscriber<TNotification>(this, std::bind(callback, static_cast<TClass*>(this)));
    }

    template<typename TNotification,
             typename TMethod,
             typename std::enable_if<!std::is_member_function_pointer<TMethod>::value &&
                                     std::is_convertible<TMethod, std::function<void()>>::value, int>::type = 0>
    void Subscribe(TMethod&& callback)
    {
        Hub::AddSubscriber<TNotification>(this, std::move(callback));
    }
};

#endif
