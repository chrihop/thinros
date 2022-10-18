#ifndef _THINROS_LINUX_H_
#define _THINROS_LINUX_H_

struct thinros_ipc
{
    int fd;
    struct topic_partition_t* par;
};

#ifdef __cplusplus
extern "C" {
#endif

struct thinros_ipc* thinros_bind(void);

void thinros_unbind(struct thinros_ipc* ipc);

#ifdef __cplusplus
}
#endif

#endif /* !_THINROS_LINUX_H_ */
