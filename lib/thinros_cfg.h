#ifndef _THINROS_CFG_H_
#define _THINROS_CFG_H_

#define _1k				(1024)
#define _1m				(_1k * _1k)
#define _1g				(_1k * _1m)

#define NONSECURE_PARTITION_LOC		((unsigned long) (4llu * _1g))
#define NONSECURE_PARTITION_SIZE	((unsigned long) (64llu * _1m))

#define TOPIC_BUFFER_SIZE			(16 * _1m)
#define MAX_MESSAGE_SIZE			(4096lu)
#define MAX_TOPICS					(64lu)
#define MAX_TOPICS_SUBSCRIBE		(16lu) /* max number of topics allows to subscribe for each node */
#define MAX_TOPICS_PUBLISH			(16lu)
#define TOPIC_NAME_SIZE				(32lu)
#define MAX_RING_ELEMS				(32lu) /* max number of elements a topic could buffer */
#define NODE_NAME_SIZE				(32lu)
#define PADDING_BYTES				(32lu)
#define MAX_PARTITIONS				(2lu)
#define INVALID_TOPIC_UUID			(0lu)

extern struct topic_namespace_t topic_namespace;


#ifndef __cplusplus
#ifndef static_assert
#define static_assert   _Static_assert
#endif
#endif

#define msg_check(type)	\
	static_assert(sizeof(type) <= MAX_MESSAGE_SIZE, "The size of "#type" is too long.")

/* -- automatically generated -- */
struct msg_float32_t
{
	float	value;
};

typedef struct msg_float32_t		msg_steer_t;
typedef struct msg_float32_t		msg_throttle_t;

typedef struct msg_lidar_t
{
	unsigned int	size;
	unsigned char	value[4000];
} msg_lidar_t;

msg_check(msg_steer_t);
msg_check(msg_throttle_t);
msg_check(msg_lidar_t);
/* ---- */


#endif /* !_THINROS_CFG_H_ */
