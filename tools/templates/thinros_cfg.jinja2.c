#include "thinros_core.h"

struct topic_namespace_t topic_namespace =
{
    .n = {{data.topics | length}},
    .topic = {
{% for name, t in topics.items() %}
        {.name = "{{t.name}}", .uuid = {{t.uuid}}, .length = {{t.queue_length}}, .elem_sz = sizeof(msg_{{t.name}}_t), .pub_pars = {{t.publish_pars | length}}},
{% endfor %}
    },
};
