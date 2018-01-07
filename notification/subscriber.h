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
