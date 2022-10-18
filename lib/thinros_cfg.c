#include "thinros_core.h"

struct topic_namespace_t topic_namespace =
{
	.n = 3,
	.topic = {
		{.name = "drv_steer",     .uuid = 1, .length = 32, .elem_sz = sizeof(msg_steer_t)},
		{.name = "drv_throttle",  .uuid = 2, .length = 32, .elem_sz = sizeof(msg_throttle_t)},
		{.name = "fwd_scan",      .uuid = 3, .length = 16, .elem_sz = sizeof(msg_lidar_t)},
		/* ... (MAX_TOPICS) */
	},
};
