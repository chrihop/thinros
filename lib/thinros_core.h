#ifndef _THINROS_CORE_H_
#define _THINROS_CORE_H_

#include "thinros_cfg.h"

#define _in
#define _out

/* mark a function that can only be accessed by the secure world */
#define __secure

#ifndef MODULE

#if defined(_STD_LIBC_)

#ifndef __cplusplus
# include <stdatomic.h>
#else
# include <atomic>
# define _Atomic(X) std::atomic< X >
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <execinfo.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define atomic_t(type)			volatile _Atomic(type) __attribute__ ((aligned (sizeof(unsigned long))))
#define gcc_packed		__attribute__((packed))
#define gcc_inline		inline __attribute__((always_inline))
#define gcc_aligned(mult)		__attribute__((aligned(mult)))
#define gcc_4k_aligned			gcc_aligned(4096)
#define likely(x)		__builtin_expect(!!(x), 1)
#define unlikely(x)		__builtin_expect(!!(x), 0)
#define info			printf
#define NULL_PTR		(0lu)

#define TRUE			true
#define FALSE			false

gcc_inline void
trace(void)
{
    void*  callstack[32];
    int    i, frames = backtrace(callstack, 32);
    char** strs = backtrace_symbols(callstack, frames);
    for (i = 0; i < frames; ++i)
    {
        printf("  %s\n", strs[i]);
    }
    free(strs);
}

gcc_inline unsigned long long
time_ns(void)
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (unsigned long long)tp.tv_sec * 1000000000ull
         + (unsigned long long)tp.tv_nsec;
}

#define ASSERT(x)                                                           \
    do                                                                      \
    {                                                                       \
        if (unlikely(!(x)))                                                 \
        {                                                                   \
            info("assertion failed (" __FILE__ ":%d): %s\n", __LINE__, #x); \
            trace();                                                        \
            exit(0);                                                        \
        }                                                                   \
    } while (0)

#define WARN(...)  info("[W] " __VA_ARGS__)
#define debug(...) info("[D] " __VA_ARGS__)
#define PANIC(...)                \
    do                            \
    {                             \
        info("[P] " __VA_ARGS__); \
        trace();                  \
        exit(0);                  \
    } while (0)

#define MIN(a, b)           \
    ({                      \
        typeof(a) _a = (a); \
        typeof(b) _b = (b); \
        _a > _b ? _b : _a;  \
    })

#define MAX(a, b)           \
    ({                      \
        typeof(a) _a = (a); \
        typeof(b) _b = (b); \
        _a > _b ? _a : _b;  \
    })

#else

#include <lib/common.h>
#define info   KERN_INFO
#define PANIC  KERN_PANIC
#define ASSERT KERN_ASSERT
#define WARN   KERN_WARN

#define true TRUE
#define false FALSE

#include <lib/atomic.h>
#include <lib/string.h>
#include <lib/sleep.h>

#endif /* STD_LIBC */

#else /* linux kernel */

#include <linux/atomic.h>
#include <linux/stddef.h>

#define atomic_t(type) \
    volatile _Atomic(type) __attribute__((aligned(sizeof(unsigned long))))
#define gcc_packed        __attribute__((packed))
#define gcc_inline        inline __attribute__((always_inline))
#define gcc_aligned(mult) __attribute__((aligned(mult)))
#define gcc_4k_aligned    gcc_aligned(4096)
#define likely(x)         __builtin_expect(!!(x), 1)
#define unlikely(x)       __builtin_expect(!!(x), 0)
#define NULL_PTR          (0lu)

#define TRUE  true
#define FALSE false

#endif /* linux kernel */

enum topic_data_status_t
{
    TOPIC_EMPTY = 0,
    TOPIC_READY = 1,
};

struct topic_data_t
{
    enum topic_data_status_t status;
    size_t                   timestamp;
    uint8_t                  data[];
};

/*
 *                                        ─┐
 *                          next_avail()   │
 *                             │           │ writer
 *                           ┌─▼─┐         │
 *                           │ E ├─┐      ─┘
 *                           └───┘ │
 *       tail◄───  n ───►head      │ append()
 *         │               │       │
 * ──────┬─▼─┬───┬───┬───┬─▼─┐     ▼
 *       │ R │ E │ R │ E │ E │ topic_ring
 * ──────┴─▲─┴───┴─▲─┴───┴───┘
 *         │       │
 *   read_tail     read_head              ─┐
 *         │       │                       │
 * ──────┬─▼─┬───┬─▼─┐                     │ reader
 *       │ Y │ N │ N │         read_ring   │
 * ──────┴───┴───┴───┘                    ─┘
 *
 * E: TOPIC_EMPTY
 * R: TOPIC_READY
 * Y: READER_HAS_READ
 * N: READER_NOT_READ
 */
struct topic_ring_t
{
    atomic_t(size_t) head;
    size_t              n;        /* total number of elements */
    size_t              elem_sz;  /* size of each element */
    struct topic_data_t buffer[]; /* buffer, size = n * size */
};

#define of(ring, index)                                  \
    (struct topic_data_t*)((unsigned long)(ring)->buffer \
                           + (index) * (ring)->elem_sz)

struct topic_writer_t
{
    size_t               index;
    struct topic_ring_t* ring;
};

enum topic_reader_data_status_t
{
    READER_NOT_READ = 0,
    READER_HAS_READ,
};

struct topic_reader_t
{
    size_t               read_head;
    size_t               read_tail;
    size_t               index;     /* current reading index */
    size_t               timestamp; /* current reading timestamp */
    struct topic_ring_t* ring;
    enum topic_reader_data_status_t
        read_ring[MAX_RING_ELEMS]; /* length = ring->n */
};

#define TOPIC_RING_DEFINE(name, length, elem_type) \
    unsigned char                                  \
        name[(length) * (sizeof(struct topic_data_t) + sizeof(elem_type))]

struct linear_allocator_t
{
    size_t size;
    atomic_t(size_t) brk;
    uintptr_t memory;
};

struct topic_namespace_item_t
{
    const char* const name;
    const size_t      uuid;
    const size_t      length;
    const size_t      elem_sz;
};

struct topic_namespace_t
{
    size_t                        n;
    struct topic_namespace_item_t topic[MAX_TOPICS];
};

typedef size_t relative_addr_t;

struct topic_registry_item_t
{
    size_t          uuid;
    bool            to_publish;
    bool            to_subscribe;
    relative_addr_t local_ring;
    relative_addr_t external_ring;
};

struct topic_registry_t
{
    size_t                       n;
    struct topic_registry_item_t topic[MAX_TOPICS];
};

/**
 * Notice: do not instantiate to a local variable
 */
struct topic_partition_t
{
    size_t                    partition_id;
    struct topic_registry_t   registry;
    struct linear_allocator_t allocator;
    uint8_t                   topic_buffer[TOPIC_BUFFER_SIZE];
} gcc_4k_aligned;

struct publisher_t
{
    size_t                    topic_uuid;
    struct topic_partition_t* partition;
    struct topic_writer_t     writer;
};

typedef void (*thinros_callback_on_t)(void* data);

struct subscriber_t
{
    size_t                topic_uuid;
    thinros_callback_on_t callback;
    struct topic_reader_t local_reader;
    struct topic_reader_t external_reader;
};

struct node_handle_t
{
    atomic_t(bool) running;
    struct topic_partition_t* par;
    char                      node_name[NODE_NAME_SIZE];
    size_t                    n_subscribers;
    struct subscriber_t*      subscribers[MAX_TOPICS_SUBSCRIBE];
    uint8_t                   buffer[MAX_MESSAGE_SIZE];
};

/**
 *                  ┌─────── replicator ─────┐
 *                  │                        │
 * ( Local Ring )──►r-reader───┐
 *                             │
 *                             │
 * ( Local Ring )──►r-reader ──┼─► r-writer ───►( External Ring )
 *                             │
 *     ...                     │
 *                             │
 * ( Local Ring )──►r-reader───┘
 */
struct topic_replicator_t
{
    size_t                topic_uuid;
    size_t                n_sources;
    struct topic_reader_t sources[MAX_PARTITIONS];
    struct topic_writer_t destination;
};

struct master_record_t
{
    struct topic_partition_t* address;
    struct topic_replicator_t replicators[MAX_TOPICS];
    size_t                    n_replicators;
};

struct thinros_master_t
{
    size_t                 n_partitions;
    struct master_record_t partitions[MAX_PARTITIONS];
};

enum thinros_spin_type_t
{
    SPIN_ONCE = 0,
    SPIN_FOREVER,
    SPIN_YIELD,
    SPIN_TIMEOUT,

    MAX_SPIN_TYPES
};

#ifdef __cplusplus
extern "C" {
#endif
/* -- debug functions -- */
void topic_ring_print(struct topic_ring_t * r);
void topic_ring_print_details(struct topic_ring_t * r);

void topic_writer_print(struct topic_writer_t * wr);
void topic_reader_print(struct topic_reader_t *rd);

void thinros_subscriber_print(struct subscriber_t * s);
void thinros_publisher_print(struct publisher_t * p);
void thinros_node_print(struct node_handle_t * n);

void topic_partition_print(struct topic_partition_t * par);
void thinros_master_print(struct thinros_master_t * m);
/*-- end of debug functions --*/

/* -- topic ring -- */
void topic_ring_init(struct topic_ring_t * p_ring, size_t n, size_t elem_sz);
size_t topic_ring_alloc(struct topic_ring_t *r);

void topic_writer_init(struct topic_writer_t *w, struct topic_ring_t *r);
struct topic_data_t * topic_writer_next_avail(struct topic_writer_t * w);
void topic_writer_complete(struct topic_writer_t * w);
struct topic_data_t * topic_writer_write(struct topic_writer_t * w, void * src, size_t sz);

void topic_reader_init(struct topic_reader_t * rd, struct topic_ring_t *r);
struct topic_data_t * topic_reader_read_next(struct topic_reader_t *rd);
struct topic_data_t * topic_reader_read_eager(struct topic_reader_t *rd);
bool topic_reader_complete(struct topic_reader_t * rd);
bool topic_reader_read(struct topic_reader_t * rd, void * dest, size_t sz);
size_t topic_reader_read_all(struct topic_reader_t * rd, void * buffer, size_t sz, thinros_callback_on_t callback);
size_t topic_ring_copy(struct topic_reader_t * rd, struct topic_writer_t * wr);
/* -- end of topic ring -- */

/* -- linear allocator -- */
void linear_allocator_init(struct linear_allocator_t * la, size_t total_sz);
uintptr_t linear_allocator_alloc(struct linear_allocator_t * la, size_t sz);
/* ---- */

/* -- partition -- */
struct topic_namespace_item_t * topic_namespace_query_by_name(char * name);
struct topic_namespace_item_t * topic_namespace_query_by_uuid(size_t uuid);

struct topic_ring_t * get_local_ring(struct topic_partition_t * par, struct topic_registry_item_t * topic);
struct topic_ring_t * get_external_ring(struct topic_partition_t * par, struct topic_registry_item_t * topic);

struct topic_registry_item_t * topic_registry_query(struct topic_registry_t * reg, size_t uuid);
void topic_partition_init(struct topic_partition_t * par);
void topic_nonsecure_partition_init(struct topic_partition_t* par);
struct topic_registry_item_t * topic_partition_get(struct topic_partition_t * par, size_t uuid);
struct topic_registry_item_t * topic_partition_get_by_name(struct topic_partition_t *par, char *topic_name);
void thinros_node(struct node_handle_t * n, struct topic_partition_t * par, char * node_name);
void thinros_advertise(_out struct publisher_t *publisher,
					   _in struct node_handle_t *n, _in char *topic_name);
void thinros_publish(_in struct publisher_t *publisher, _in void *message,
					 _in size_t sz);
void thinros_subscribe(_in struct subscriber_t * subscriber,
					   _in struct node_handle_t * n, _in char * topic_name,
					   _in thinros_callback_on_t callback);
void thinros_spin(_in struct node_handle_t * n,
					_in enum thinros_spin_type_t type,
					_in void (*yield)(void),
					_in uint64_t nanoseconds);
/* ---- */

/* -- master (secure only) -- */
__secure void thinros_master_init(struct thinros_master_t * m);
__secure void thinros_master_add(struct thinros_master_t * m, struct topic_partition_t * par);
__secure void thinros_master_build(struct thinros_master_t * m);
__secure void thinros_master_switch_to(struct thinros_master_t * m, size_t partition_idx);
/* ---- */

#ifdef __cplusplus
}
#endif

#endif /* !_THINROS_CORE_H_ */
