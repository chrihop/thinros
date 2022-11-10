#ifndef _THINROS_CFG_H_
#define _THINROS_CFG_H_

#define _1k				(1024)
#define _1m				(_1k * _1k)
#define _1g				(_1k * _1m)

#define PM_NONSECURE_PARTITION_LOC  (0x80000000llu)
#define PM_NONSECURE_PARTITION_SIZE (64llu * _1m)

#define NONSECURE_PARTITION_LOC		((unsigned long) (PM_NONSECURE_PARTITION_LOC))
#define NONSECURE_PARTITION_SIZE	((unsigned long) (PM_NONSECURE_PARTITION_SIZE))

#define TOPIC_BUFFER_SIZE			(4 * _1m)
#define MAX_MESSAGE_SIZE			(8192lu)
#define MAX_TOPICS					(64lu)
#define MAX_TOPICS_SUBSCRIBE		(16lu) /* max number of topics allows to subscribe for each node */
#define MAX_TOPICS_PUBLISH			(16lu)
#define TOPIC_NAME_SIZE				(32lu)
#define MAX_RING_ELEMS				(32lu) /* max number of elements a topic could buffer */
#define NODE_NAME_SIZE				(32lu)
#define PADDING_BYTES				(32lu)
#define MAX_PARTITIONS				(16lu)
#define INVALID_TOPIC_UUID			(0lu)

extern struct topic_namespace_t topic_namespace;


#ifndef __cplusplus
#ifndef static_assert
#define static_assert   _Static_assert
#endif
#endif

#define msg_check(type)	\
	static_assert(sizeof(type) <= MAX_MESSAGE_SIZE, "The size of "#type" is too long.")


/* SAFETY CONTROLLER */
struct msg_float32_t
{
	float	value;
};

typedef struct msg_float32_t		msg_steer_t;
typedef struct msg_float32_t		msg_throttle_t;

typedef struct msg_lidar_t
{
	unsigned int	size;
	unsigned char	value[6132];
} msg_lidar_t;

msg_check(msg_steer_t);
msg_check(msg_throttle_t);
msg_check(msg_lidar_t);

/* Mavlink Gateway */

#define MAX_MAVLINK_MSG_SIZE		(280lu)

typedef struct msg_mavlink_t
{
    unsigned long len;
    unsigned char data[MAX_MAVLINK_MSG_SIZE];
} msg_mavlink_t;

msg_check(msg_mavlink_t);

/* Crypto Server */
typedef struct encryption_service_t
{
    unsigned char id;
    unsigned long len;
    unsigned char data[128];
} encryption_service_t;

msg_check(encryption_service_t);

/* BENCHMARK */
typedef enum
{
    MSG_BENCHMARK_RESULT_DATA_POINT
} msg_benchmark_result_type_t;


typedef struct
{
    unsigned long long int times[128];
} msg_benchmark_results_t;


typedef struct
{
    unsigned int value;
} msg_benchmark_sz_4_t;

typedef struct
{
    unsigned int value[4];
} msg_benchmark_sz_16_t;

typedef struct
{
    unsigned int value[16];
} msg_benchmark_sz_64_t;

typedef struct
{
    unsigned int value[64];
} msg_benchmark_sz_256_t;

typedef struct
{
    unsigned int value[256];
} msg_benchmark_sz_1K_t;

typedef struct
{
    unsigned int value[1024];
} msg_benchmark_sz_4K_t;


#endif /* !_THINROS_CFG_H_ */
