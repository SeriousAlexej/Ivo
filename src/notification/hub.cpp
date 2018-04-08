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
#include "notification/hub.h"

std::unordered_map<std::type_index, std::unordered_map<Subscriber*, std::vector<std::function<void()>>>> Hub::g_subscriptions;

void Hub::RemoveSubscriber(Subscriber* subscriber)
{
    for(auto it = g_subscriptions.begin(); it != g_subscriptions.end(); it++)
        (*it).second.erase(subscriber);
}
