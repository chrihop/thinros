/**
 * Node interface definition for {{ node.name }}
 *
 * @note This file is generated by thinros. Do not edit it manually.
 * @file {{ node.name }}.h
 * - node: {{ node.name }}
 * - partition: {{ node.partition.par_id }}
 * - network: {{ misc.network }}
 * @version {{ misc.version }}
 * @date {{ misc.date }}
 */

#include "{{ node.name }}.h"

/* startup aggregate */
{% if aggregate.startup | length > 0 %}
{% set use_aggregated_startup = true %}
CALLBACKS_ON_OBJ(USE_STARTUP({{ node.name }}),
{% for s in aggregate.startup %}
    {{ s.name }},
{% endfor %}
);
{% endif %}

/* shutdown aggregate */
{% if aggregate.shutdown | length > 0 %}
{% set use_aggregated_shutdown = true %}
CALLBACKS_ON_OBJ(USE_SHUTDOWN({{ node.name }}),
{% for s in aggregate.shutdown %}
    {{ s.name }},
{% endfor %}
);
{% endif %}

/* spin aggregate */
{% if aggregate.spin | length > 0 %}
{% set use_aggregated_spin = true %}
CALLBACKS_ON_SPIN(USE_SPIN({{ node.name }}),
{% for s in aggregate.spin %}
    {{ s.name }},
{% endfor %}
);
{% endif %}

/* message handler aggregate */
{% for t, content in aggregate.subscribe.items() %}
{% if content['callbacks'] | length > 0 %}
{% if content['entry'].name == 'default' %}
CALLBACKS_ON_MSG(USE_MESSAGE_HANDLER({{ node.name }}, {{ t }}),
{% else %}
CALLBACKS_ON_MSG({{ content['entry'].name }},
{% endif %}
{% for c in content['callbacks'] %}
    {{ c.name }},
{% endfor %}
);
{% endif %}
{% endfor %}

/* node definition */
THINROS_OBJ(
    {{ node.name }},
{% if node.startup_entry.name == 'default' %}
    USE_STARTUP({{ node.name }}),
{% else %}
    {{ node.startup_entry.name }},
{% endif %}
{% if node.shutdown_entry.name == 'default' %}
    USE_SHUTDOWN({{ node.name }}),
{% else %}
    {{ node.shutdown_entry.name }},
{% endif %}
{% if node.spin_entry.name == 'default' %}
    USE_SPIN({{ node.name }}),
{% else %}
    {{ node.spin_entry.name }},
{% endif %}
    ADVERTISE(
{% for t in node.publish %}
    "{{ t.name }}",
{% endfor %}
    ),
    SUBSCRIBE(
{% for t, content in aggregate.subscribe.items() %}
        ON_MESSAGE({{ t }}, 
{%- if content['entry'].name == 'default' -%}
    USE_MESSAGE_HANDLER({{ node.name }}, {{ t }}),
{%- else -%}
    {{ content['entry'].name }}),
{% endif %}

{% endfor %}
    ),
);