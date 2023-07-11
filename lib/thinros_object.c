#include "thinros_object.h"

thinros_object_runtime_t __runtime;
extern thinros_object_runtime_t runtime __attribute((alias("__runtime")));


int thinros_main(thinros_object_t * obj, int argc, char ** argv)
{
    ASSERT(obj != NULL);
    ASSERT(obj->name != NULL);

    memset(&runtime, 0, sizeof(runtime));
    runtime.obj = obj;
#ifdef _STD_LIBC_

#ifdef _THINROS_UNIT_TESTS_
    runtime.ipc = malloc(sizeof(struct thinros_ipc));
    runtime.ipc->fd = -1;
    struct topic_partition_t * par = malloc(sizeof(struct topic_partition_t));
    runtime.ipc->par = par;
    memset(par, 0, sizeof(struct topic_partition_t));
    struct thinros_master_t * master = malloc(sizeof(struct thinros_master_t));
    memset(master, 0, sizeof(struct thinros_master_t));
    thinros_master_init(master);
    topic_partition_init(par);
    thinros_master_add(master, par);
    topic_nonsecure_partition_init(par);

#else
    runtime.ipc = thinros_bind();
    if (runtime.ipc == NULL)
    {
        PANIC("thinros_bind() failed\n");
        return (EXIT_FAILURE);
    }

    struct topic_partition_t * par = runtime.ipc->par;
    if (par->status == PARTITION_UNINITIALIZED)
    {
        topic_partition_init(par);
    }
    topic_nonsecure_partition_init(par);
#endif
#else
    // TODO

#endif
    thinros_node(&runtime.node, par, (char *) obj->name);

    for (size_t i = 0; i < MAX_TOPICS_PUBLISH; i++)
    {
        if (obj->advertise[i] != NULL)
        {
            thinros_advertise(&runtime.pub[i], &runtime.node, (char *) obj->advertise[i]);
        }
    }

    for (size_t i = 0; i < MAX_TOPICS_SUBSCRIBE; i++)
    {
        if (obj->subscribe[i].name != NULL && obj->subscribe[i].on_message != NULL)
        {
            thinros_subscribe(&runtime.sub[i], &runtime.node, (char *) obj->subscribe[i].name,
                obj->subscribe[i].on_message);
        }
    }

#if defined(_STD_LIBC_) && defined(_THINROS_UNIT_TESTS_)
    thinros_master_build(master);
#endif

    if (obj->startup != NULL)
    {
        obj->startup(obj);
    }

    bool succ = true;
    size_t spin_times = 0;
    while (likely(succ))
    {
        thinros_spin(&runtime.node, SPIN_ONCE, NULL, 0llu);

        if (obj->spin != NULL)
        {
            succ &= obj->spin(obj, spin_times);

#ifdef _STD_LIBC_
            usleep(20000);
#endif

        }
        spin_times++;
    }

    if (obj->shutdown != NULL)
    {
        obj->shutdown(obj);
    }

#ifdef _STD_LIBC_
#ifdef _THINROS_UNIT_TESTS_
    free(runtime.ipc->par);
    free(runtime.ipc);
#else
    thinros_unbind(runtime.ipc);
#endif
#else
    // TODO
#endif

    return 0;
}
