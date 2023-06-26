#ifndef _THINROS_CFG_H_
#define _THINROS_CFG_H_

/**
 * Generated from:
 * - {{misc.topics_json}}
 * - {{misc.platform_json}}
 * @date {{misc.date}}
 * @version {{misc.version}}
 */

/**
 * Platform base configuration
 */
#define NONSECURE_PARTITION_LOC		((unsigned long long) ({{data.nonsecure_partition.base}}))
#define NONSECURE_PARTITION_SIZE	((unsigned long long) ({{data.nonsecure_partition.size}}llu))

#define TOPIC_BUFFER_SIZE			({{data.limits['topic-buffer-size']}}lu)
#define MAX_MESSAGE_SIZE			({{data.limits['max-message-size']}}lu)
#define MAX_TOPICS					({{data.limits['max-topics']}}lu)
#define MAX_TOPICS_SUBSCRIBE		({{data.limits['max-topics-subscribe']}}lu) /* max number of topics allows to subscribe for each node */
#define MAX_TOPICS_PUBLISH			({{data.limits['max-topics-publish']}}lu)
#define TOPIC_NAME_SIZE				({{data.limits['topic-name-size']}}lu)
#define MAX_RING_ELEMS				({{data.limits['max-ring-elements']}}lu) /* max number of elements a topic could buffer */
#define NODE_NAME_SIZE				({{data.limits['node-name-size']}}lu)
#define PADDING_BYTES				({{data.limits['padding']}}lu)
#define MAX_PARTITIONS				({{data.limits['max-partitions']}}lu)
#define INVALID_TOPIC_UUID			(0lu)

/**
 * Topics configuration
 */
extern struct topic_namespace_t topic_namespace;

#ifndef static_assert
#ifdef __cplusplus
#define static_assert static_assert
#else
#define static_assert _Static_assert
#endif /* __cplusplus */
#endif /* static_assert */

#define MSG_HEADER_SIZE     (8)

#define msg_size_check(msg_type) \
    static_assert(sizeof(msg_type) + MSG_HEADER_SIZE <= MAX_MESSAGE_SIZE, \
        "Message size exceeds MAX_MESSAGE_SIZE")

{% for name, t in topics.items() %}
typedef struct msg_{{name}}_t
{
{% for def in t.c_definition %}
    {{def}};
{% endfor %}
} msg_{{name}}_t;

msg_size_check(msg_{{name}}_t);

{% endfor %}

#endif /* !_THINROS_CFG_H_ */
