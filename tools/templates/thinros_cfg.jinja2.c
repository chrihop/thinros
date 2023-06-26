#include "thinros_core.h"

static struct topic_namespace_item_t topic_namespace_items[] =
{
{% for name, t in topics.items() %}
    {.name = "{{t.name}}", .uuid = {{t.uuid}}, .length = {{t.queue_length}}, .elem_sz = sizeof(msg_{{t.name}}_t), .pub_pars = {{t.publish_pars | length}}},
{% endfor %}
};

struct topic_namespace_t topic_namespace =
{
    .n = sizeof(topic_namespace_items) / sizeof(topic_namespace_items[0]),
    .topic = topic_namespace_items,
};
