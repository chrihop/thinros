#include "thinros_core.h"

#if (TROS_SCENARIO_SAFETY_CONTROLLER)
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
#endif /* TROS_SCENARIO_SAFETY_CONTROLLER */

#if (TROS_SCENARIO_BENCH_INTRA_PARTITION)
struct topic_namespace_t topic_namespace =
{
	.n = 7,
	.topic = {
		{.name = "benchmark_4",       .uuid = 0, .length = 16, .elem_sz = sizeof(msg_benchmark_sz_4_t)},
		{.name = "benchmark_16",      .uuid = 1, .length = 16, .elem_sz = sizeof(msg_benchmark_sz_16_t)},
		{.name = "benchmark_64",      .uuid = 2, .length = 16, .elem_sz = sizeof(msg_benchmark_sz_64_t)},
		{.name = "benchmark_256",     .uuid = 3, .length = 16, .elem_sz = sizeof(msg_benchmark_sz_256_t)},
		{.name = "benchmark_1K",      .uuid = 4, .length = 16, .elem_sz = sizeof(msg_benchmark_sz_1K_t)},
		{.name = "benchmark_4K",      .uuid = 5, .length = 16, .elem_sz = sizeof(msg_benchmark_sz_4K_t)},
		{.name = "benchmark_results", .uuid = 6, .length = 4, .elem_sz = sizeof(msg_benchmark_results_t)},
	},
};
#endif /* TROS_SCENARIO_BENCH_INTRA_PARTITION */

#if (TROS_SCENARIO_SECURE_GATEWAY)
struct topic_namespace_t topic_namespace =
{
    .n = 4,
    .topic = {
        {.name = "mav_gateway_out", .uuid = 0, .length = 16, .elem_sz = sizeof(msg_mavlink_t)},
        {.name = "mav_gateway_in",  .uuid = 1, .length = 16, .elem_sz = sizeof(msg_mavlink_t)},
        {.name = "cipher_text", .uuid = 2, .length = 16, .elem_sz = sizeof(encryption_service_t)},
        {.name = "enc_request",  .uuid = 3, .length = 16, .elem_sz = sizeof(encryption_service_t)},
    },
};
#endif /* TROS_SCENARIO_SECURE_GATEWAY */
