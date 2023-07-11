#ifndef _THINROS_OBJECT_H_
#define _THINROS_OBJECT_H_

#include "thinros_core.h"

#ifdef _STD_LIBC_
#include "thinros_linux.h"
#endif

struct thinros_object_t;

typedef void (*thinros_object_startup_t)(struct thinros_object_t* obj);
typedef void (*thinros_object_shutdown_t)(struct thinros_object_t* obj);
typedef bool (*thinros_object_spin_t)(struct thinros_object_t* obj, size_t n);
typedef void (*thinros_object_on_message_t)(void* data);

typedef struct thinros_object_t
{
    const char*               name;
    thinros_object_startup_t  startup;
    thinros_object_shutdown_t shutdown;
    thinros_object_spin_t     spin;
    const char*               advertise[MAX_TOPICS_PUBLISH];
    struct
    {
        const char*                 name;
        thinros_object_on_message_t on_message;
    } subscribe[MAX_TOPICS_SUBSCRIBE];
} thinros_object_t;

typedef struct thinros_object_runtime_t
{
    struct thinros_object_t* obj;
    struct node_handle_t     node;
    struct publisher_t       pub[MAX_TOPICS_PUBLISH];
    struct subscriber_t      sub[MAX_TOPICS_SUBSCRIBE];
#ifdef _STD_LIBC_
    struct thinros_ipc* ipc;
#endif
} thinros_object_runtime_t;

typedef void (*thinros_callback_msg_t)(void* data);
typedef void (*thinros_callback_obj_t)(struct thinros_object_t* obj);
typedef bool (*thinros_callback_obj_size_t)(struct thinros_object_t* obj, size_t n);

typedef union
{
    void*                       any;
    thinros_callback_msg_t      on_msg;
    thinros_callback_obj_t      on_obj;
    thinros_callback_obj_size_t on_spin;
} thinros_callback_t;

typedef struct thinros_object_callbacks_aggregate_t
{
    thinros_callback_t          *callbacks;
    size_t                       size;
} thinros_object_callbacks_aggregate_t;

typedef struct thinros_namespace_t
{
    size_t runtime_publisher[MAX_TOPICS_PUBLISH];
    size_t runtime_subscriber[MAX_TOPICS_SUBSCRIBE];
} thinros_namespace_t;

#define _ADVERTISE(...) \
    { __VA_ARGS__, NULL }

#define ADVERTISE(...) \
    _ADVERTISE(__VA_ARGS__)

#define ON_MESSAGE(name, func) \
    { #name, func }

#define _SUBSCRIBE(...) \
    { __VA_ARGS__, {NULL, NULL} }

#define SUBSCRIBE(...) \
    _SUBSCRIBE(__VA_ARGS__)

#define THINROS_OBJ(_name, _startup, _shutdown, _spin, _advertise, _subscribe) \
    thinros_object_t _name = {                                                 \
        .name      = #_name,                                                   \
        .startup   = _startup,                                                 \
        .shutdown  = _shutdown,                                                \
        .spin      = _spin,                                                    \
        .advertise = _advertise,                                               \
        .subscribe = _subscribe,                                               \
    }

#define THINROS_OBJ_DECLARE(_name) \
    extern thinros_object_t _name

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#define _CALLBACKS_LIST_DEFINE(_name, _type, ...) \
    static thinros_callback_ ## _type ## _t __callbacks_##_name[] = { __VA_ARGS__ }

#define _CALLBACKS_AGGR_DEFINE(_name) \
    static thinros_object_callbacks_aggregate_t __aggrigate_ ## _name = {                \
                .callbacks = (thinros_callback_t *) __callbacks_##_name,                        \
                .size      = ARRAY_SIZE(__callbacks_##_name),            \
    }

#define _CALLBACKS_AGGR_FUNC_ON_MSG(_name) \
    static void _name (void* data) \
    { \
        thinros_object_callbacks_aggregate_t* aggr = &__aggrigate_ ## _name; \
        for (size_t i = 0; i < aggr->size; i++) \
        { \
            aggr->callbacks[i].on_msg(data); \
        } \
    }

#define _CALLBACKS_AGGR_FUNC_ON_OBJ(_name) \
    static void _name (struct thinros_object_t* obj) \
    { \
        thinros_object_callbacks_aggregate_t* aggr = &__aggrigate_ ## _name; \
        for (size_t i = 0; i < aggr->size; i++) \
        { \
            aggr->callbacks[i].on_obj(obj); \
        } \
    }

#define _CALLBACKS_AGGR_FUNC_ON_SPIN(_name) \
    static bool _name (struct thinros_object_t* obj, size_t n) \
    { \
        thinros_object_callbacks_aggregate_t* aggr = &__aggrigate_ ## _name; \
        bool succ = true;                                    \
        for (size_t i = 0; i < aggr->size; i++) \
        { \
            succ &= aggr->callbacks[i].on_spin(obj, n); \
        }                                   \
        return succ; \
    }

#define _CALLBACKS_ON_MSG(_name, ...) \
    _CALLBACKS_LIST_DEFINE(_name, msg, __VA_ARGS__); \
    _CALLBACKS_AGGR_DEFINE(_name); \
    _CALLBACKS_AGGR_FUNC_ON_MSG(_name)

#define _CALLBACKS_ON_OBJ(_name, ...) \
    _CALLBACKS_LIST_DEFINE(_name, obj, __VA_ARGS__); \
    _CALLBACKS_AGGR_DEFINE(_name); \
    _CALLBACKS_AGGR_FUNC_ON_OBJ(_name)

#define _CALLBACKS_ON_SPIN(_name, ...) \
    _CALLBACKS_LIST_DEFINE(_name, obj_size, __VA_ARGS__); \
    _CALLBACKS_AGGR_DEFINE(_name); \
    _CALLBACKS_AGGR_FUNC_ON_SPIN(_name)

#define CALLBACKS_ON_MSG(_name, ...) _CALLBACKS_ON_MSG(_name, __VA_ARGS__)
#define CALLBACKS_ON_OBJ(_name, ...) _CALLBACKS_ON_OBJ(_name, __VA_ARGS__)
#define CALLBACKS_ON_SPIN(_name, ...) _CALLBACKS_ON_SPIN(_name, __VA_ARGS__)

#define STARTUP(name) \
    static void __on_startup_##name(thinros_object_t* obj)

#define _USE_STARTUP(name) \
    __on_startup_##name

#define USE_STARTUP(name) \
    _USE_STARTUP(name)

#define SHUTDOWN(name) \
    static void __on_shutdown_##name(thinros_object_t* obj)

#define _USE_SHUTDOWN(name) \
    __on_shutdown_##name

#define USE_SHUTDOWN(name) \
    _USE_SHUTDOWN(name)

#define SPIN(name, N_TIMES) \
    static bool __on_spin_##name(thinros_object_t* obj, size_t N_TIMES)

#define _USE_SPIN(name) \
    __on_spin_##name

#define USE_SPIN(name) \
    _USE_SPIN(name)

#define MESSAGE_HANDLER(_name, _topic, DATA) \
    static void __on_message_##_name##_##_topic(void* DATA)

#define _USE_MESSAGE_HANDLER(_name, _topic) \
    __on_message_ ## _name ## _ ## _topic

#define USE_MESSAGE_HANDLER(_name, _topic) \
    _USE_MESSAGE_HANDLER(_name, _topic)

#define THINROS_NS_DEFINE(_name, _publisher_map, _subscriber_map) \
    static thinros_namespace_t namespace_##_name = {                          \
        .runtime_publisher = _publisher_map,                           \
        .runtime_subscriber = _subscriber_map,                         \
    }

#define USE_NS(_name) \
    namespace_##_name

#define NS_MAP(_direction, _topic, _global_index) \
    [_direction ## _TOPIC_##_topic] = _global_index

#define NS_LIST(...) \
    { __VA_ARGS__ }

#define _NS_THINROS_PUBLISH(_namespace, _runtime, _topic, _data, _size) \
    do                                                              \
    {                                                               \
        size_t __pub_idx                                            \
            = _namespace.runtime_publisher[P_TOPIC_##_topic]; \
        struct publisher_t* __pub = &_runtime.pub[__pub_idx];       \
        thinros_publish(__pub, _data, _size);                       \
    } while (0)

#define NS_THINROS_PUBLISH(_namespace, _topic, _data, _size) \
    _NS_THINROS_PUBLISH(_namespace, __runtime, _topic, _data, _size)

#define _THINROS_PUBLISH(_runtime, _topic, _data, _size) \
    do {                                                 \
        struct publisher_t* __pub = &_runtume.pub[_topic]                                                     \
                                                         \
                                                         \
    }
#define THINROS_OBJECT_INTERFACE \
    extern thinros_object_runtime_t __runtime;

#if __cplusplus
extern "C"
{
#endif

int thinros_main(thinros_object_t * obj, int argc, char ** argv);

#if __cplusplus
};
#endif

#define THINROS_MAIN(_obj) \
    int main(int argc, char** argv) \
    { \
        return thinros_main(&_obj, argc, argv); \
    }

#define _AGGREGATE(_a, _b) _a ## _ ## _b
#define AGGR(_a, _b) _AGGREGATE(_a, _b)


#endif /* !_THINROS_OBJECT_H_ */
