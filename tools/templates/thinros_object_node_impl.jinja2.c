/**
 * Node implementation for {{ node.name }}
 *
 * @note Implement the node logic here. Be careful to be overwriting the
 * @file {{ node.name }}.h
 * - node: {{ node.name }}
 * - partition: {{ node.partition.par_id }}
 * - network: {{ misc.network }}
 * @version {{ misc.version }}
 * @date {{ misc.date }}
 */

#include "{{ node.name }}.h"

/* startup callbacks */
{% if aggregate.startup | length > 0 %}
{% for s in aggregate.startup %}
void {{ s.name }}(struct thinros_object_t * obj)
{
    // TODO: implement startup callback
    info("{{ node.name }}: startup {{ s.name }}\n");
}

{% endfor %}
{% endif %}

{% if node.startup_entry.name == 'default' %}
void USE_STARTUP({{ node.name }})(struct thinros_object_t * obj)
{
    info("{{ node.name }}: startup default\n");
}

{% else %}
void {{ node.startup_entry.name }}(struct thinros_object_t * obj)
{
    info("{{ node.name }}: startup {{ node.startup_entry.name }}\n");
}

{% endif %}

/* shutdown callbacks */
{% if aggregate.shutdown | length > 0 %}
{% for s in aggregate.shutdown %}
void {{ s.name }}(struct thinros_object_t * obj)
{
    // TODO: implement shutdown callback
    info("{{ node.name }}: shutdown {{ s.name }}\n");
}

{% endfor %}
{% endif %}

{% if node.shutdown_entry.name == 'default' %}
void USE_SHUTDOWN({{ node.name }})(struct thinros_object_t * obj)
{
    info("{{ node.name }}: shutdown default\n");
}

{% else %}
void {{ node.shutdown_entry.name }}(struct thinros_object_t * obj)
{
    info("{{ node.name }}: shutdown {{ node.shutdown_entry.name }}\n");
}

{% endif %}

/* on_spin callbacks */
{% if aggregate.spin | length > 0 %}
{% for s in aggregate.spin %}
bool {{ s.name }}(struct thinros_object_t * obj, size_t n)
{
    // TODO: implement spin callback
    info("{{ node.name }}: spin {{ s.name }}\n");
}

{% endfor %}
{% endif %}

{% if node.spin_entry.name == 'default' %}
bool USE_SPIN({{ node.name }})(struct thinros_object_t * obj, size_t n)
{
    info("{{ node.name }}: spin default\n");
}

{% else %}
bool {{ node.spin_entry.name }}(struct thinros_object_t * obj, size_t n)
{
    info("{{ node.name }}: spin {{ node.spin_entry.name }}\n");
}
{% endif %}


/* on_event callbacks */
{% for t, content in aggregate.subscribe.items() %}
/* on {{ t }} */    
{% for c in content['callbacks'] %}
void {{ c.name }}(void * data)
{
    // TODO: implement event callback
    info("{{ node.name }}: event {{ t }}: {{ c.name }}\n");
}

{% endfor %}
{% if content['entry'].name == 'default' %}
void USE_MESSAGE_HANDLER({{ node.name }}, {{ t }})(void * data)
{
    // TODO: implement event callback
    info("{{ node.name }}: event {{ t }}: default\n");
}

{% else %}
void {{ content['entry'].name }}(void * data)
{
    // TODO: implement event callback
    info("{{ node.name }}: event {{ t }}: {{ content['entry'].name }}\n");
}

{% endif %}
{% endfor %}

