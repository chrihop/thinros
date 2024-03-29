#include "../lib/thinros_core.h"
#include <stdio.h>

/*-- unit test section --*/
#define UNIT_TEST

#ifdef UNIT_TEST

struct test_data_t
{
	uint8_t arr[32];
};

static TOPIC_RING_DEFINE(test_ring, 128, struct test_data_t);
static TOPIC_RING_DEFINE(test_ring2, 128, struct test_data_t);
static struct topic_reader_t test_reader;
static struct topic_writer_t test_writer;
static struct test_data_t test_rx;

static struct topic_partition_t this_part;
static struct topic_partition_t other_part;
static struct node_handle_t test_node_a, test_node_b;
static struct subscriber_t node_b_sub;

void test_dummy_callback(void *data)
{
	ASSERT (data != NULL);
	struct test_data_t *d = data;
	size_t i;
	info("message received: ");
	for (i = 0; i < 12 && d->arr[i] != '\0'; i++)
	{
		info("%c ", d->arr[i]);
	}
	info("\n");
}

static void test_topic_ring(void)
{
	size_t idx;
	struct topic_ring_t *ring = (struct topic_ring_t *) test_ring;

	topic_ring_init(ring, 4, sizeof(struct test_data_t));
	topic_ring_print(ring);
	idx = topic_ring_alloc(ring);
	topic_ring_print(ring);
	info("get idx = %lu\n", idx);
	idx = topic_ring_alloc(ring);
	topic_ring_print(ring);
	info("get idx = %lu\n", idx);
}

static void test_topic_reader_writer(void)
{
	struct topic_ring_t *ring = (struct topic_ring_t *) test_ring;
	struct topic_writer_t *wr = &test_writer;

	topic_writer_init(wr, ring);
	topic_writer_write(wr, test_rx.arr, 32);
	topic_ring_print(ring);
	topic_writer_write(wr, test_rx.arr, 32);
	topic_ring_print(ring);
	topic_writer_write(wr, test_rx.arr, 32);
	topic_ring_print(ring);
	topic_writer_write(wr, test_rx.arr, 32);
	topic_ring_print(ring);

	struct topic_reader_t *rd = &test_reader;
	bool succ;
	topic_reader_init(rd, ring);
	topic_reader_print(rd);
	succ = topic_reader_read(rd, test_rx.arr, 32);
	info("succ %d: ", succ);
	topic_reader_print(rd);
	succ = topic_reader_read(rd, test_rx.arr, 32);
	info("succ %d: ", succ);
	topic_reader_print(rd);
	succ = topic_reader_read(rd, test_rx.arr, 32);
	info("succ %d: ", succ);
	topic_reader_print(rd);
	succ = topic_reader_read(rd, test_rx.arr, 32);
	info("succ %d: ", succ);
	topic_reader_print(rd);
	succ = topic_reader_read(rd, test_rx.arr, 32);
	info("succ %d: ", succ);
	topic_reader_print(rd);
}

static void test_topic_namespace(void)
{
	struct topic_namespace_item_t *ns;
	ns = topic_namespace_query_by_name("drv_steer");
	if (ns != NULL)
		info("ns %s %lu len %lu elem_sz %lu\n", ns->name, ns->uuid, ns->length,
			 ns->elem_sz);
	else
		info("not found\n");
	ns = topic_namespace_query_by_name("drv_throttle");
	if (ns != NULL)
		info("ns %s %lu len %lu elem_sz %lu\n", ns->name, ns->uuid, ns->length,
			 ns->elem_sz);
	else
		info("not found\n");
	ns = topic_namespace_query_by_name("fwd_scan");
	if (ns != NULL)
		info("ns %s %lu len %lu elem_sz %lu\n", ns->name, ns->uuid, ns->length,
			 ns->elem_sz);
	else
		info("not found\n");
	ns = topic_namespace_query_by_name("not_exists_topic");
	if (ns != NULL)
		info("ns %s %lu len %lu elem_sz %lu\n", ns->name, ns->uuid, ns->length,
			 ns->elem_sz);
	else
		info("not found\n");
}

static void test_partition_local(void)
{
	topic_partition_init(&this_part);
	info("now allocator at %lu (addr 0x%lx)\n", this_part.allocator.brk,
		 this_part.allocator.memory + this_part.allocator.brk);
	struct topic_registry_item_t *topic = topic_partition_get(&this_part, 1);
	struct topic_ring_t *r = get_local_ring(&this_part, topic);
	info("ring @ 0x%p ", r);
	info("now allocator at %lu (addr 0x%lx)\n", this_part.allocator.brk,
		 this_part.allocator.memory + this_part.allocator.brk);
	topic = topic_registry_query(&this_part.registry, 1);
	r = get_local_ring(&this_part, topic);
	info("query ring @ 0x%p\n", r);

	info("now allocator at %lu (addr 0x%lx)\n", this_part.allocator.brk,
		 this_part.allocator.memory + this_part.allocator.brk);
	topic = topic_partition_get(&this_part, 1);
	r = get_local_ring(&this_part, topic);
	info("ring @ 0x%p ", r);
	info("now allocator at %lu (addr 0x%lx)\n", this_part.allocator.brk,
		 this_part.allocator.memory + this_part.allocator.brk);
	topic = topic_registry_query(&this_part.registry, 1);
	r = get_local_ring(&this_part, topic);
	info("query ring @ 0x%p\n", r);

	info("now allocator at %lu (addr 0x%lx)\n", this_part.allocator.brk,
		 this_part.allocator.memory + this_part.allocator.brk);
    topic = topic_partition_get(&this_part, 2);
	r = get_local_ring(&this_part, topic);
	info("ring @ 0x%p ", r);
	info("now allocator at %lu (addr 0x%lx)\n", this_part.allocator.brk,
		 this_part.allocator.memory + this_part.allocator.brk);

    topic = topic_registry_query(&this_part.registry, 2);
	r = get_local_ring(&this_part, topic);
	info("query ring @ 0x%p\n", r);

	info("now allocator at %lu (addr 0x%lx)\n", this_part.allocator.brk,
		 this_part.allocator.memory + this_part.allocator.brk);

    topic = topic_partition_get(&this_part, 3);
	r = get_local_ring(&this_part, topic);
	info("ring @ 0x%p ", r);
	info("now allocator at %lu (addr 0x%lx)\n", this_part.allocator.brk,
		 this_part.allocator.memory + this_part.allocator.brk);

    topic = topic_registry_query(&this_part.registry, 3);
	r = get_local_ring(&this_part, topic);
	info("query ring @ 0x%p\n", r);

	info("now allocator at %lu (addr 0x%lx)\n", this_part.allocator.brk,
		 this_part.allocator.memory + this_part.allocator.brk);
    topic = topic_partition_get(&this_part, 1);
	r = get_local_ring(&this_part, topic);

	info("ring @ 0x%p ", r);
	info("now allocator at %lu (addr 0x%lx)\n", this_part.allocator.brk,
		 this_part.allocator.memory + this_part.allocator.brk);

    topic = topic_registry_query(&this_part.registry, 1);
	r = get_local_ring(&this_part, topic);
	info("query ring @ 0x%p\n", r);

	unsigned long long now = time_ns();
	printf("now: %llu\n", now);

	thinros_node(&test_node_a, &this_part, "node a");
	info("instantiate node a\n");
	thinros_node(&test_node_b, &this_part, "node b");
	info("instantiate node b\n");

	struct publisher_t node_a_pub;
	thinros_advertise(&node_a_pub, &test_node_a, "fwd_scan");
	info("node a advertise topic `fwd_scan`\n");
	thinros_node_print(&test_node_a);
	topic_ring_print(node_a_pub.writer.ring);
	thinros_publish(&node_a_pub, "abcdefg", 12);
	info("publish a message\n");
	topic_ring_print_details(node_a_pub.writer.ring);
	thinros_publish(&node_a_pub, "1234567", 12);
	info("publish a message\n");
	topic_ring_print_details(node_a_pub.writer.ring);
	thinros_subscribe(&node_b_sub, &test_node_b, "fwd_scan",
					  test_dummy_callback);
	thinros_node_print(&test_node_b);
	thinros_subscriber_print(&node_b_sub);
	thinros_spin(&test_node_b, SPIN_ONCE, NULL, 0);
	thinros_spin(&test_node_b, SPIN_ONCE, NULL, 0);
	for (size_t i = 0; i < 40; i++)
	{
		char message[12];
		sprintf(message, "msg %lu", i);
		thinros_publish(&node_a_pub, message, 12);
	}
	topic_ring_print_details(node_a_pub.writer.ring);
	thinros_spin(&test_node_b, SPIN_ONCE, NULL, 0);
}

static struct topic_reader_t test_copy_reader;
static struct topic_writer_t test_copy_writer;

/*
 *  wr                            rd
 *   |                            ^
 *   v                            |
 *   r1 -- rrd -> copy -- rwr --> r2
 * */
static void test_topic_ring_copy(void)
{
	struct topic_ring_t *r1 = (struct topic_ring_t *) test_ring;
	struct topic_ring_t *r2 = (struct topic_ring_t *) test_ring2;
	topic_ring_init(r1, 4, sizeof(struct test_data_t));
	topic_ring_init(r2, 4, sizeof(struct test_data_t));

	struct topic_reader_t *rd = (struct topic_reader_t *) &test_reader;
	struct topic_writer_t *wr = (struct topic_writer_t *) &test_writer;

	struct topic_reader_t *rrd = (struct topic_reader_t *) &test_copy_reader;
	struct topic_writer_t *rwr = (struct topic_writer_t *) &test_copy_writer;

	topic_writer_init(wr, r1);
	topic_reader_init(rd, r2);

	topic_reader_init(rrd, r1);
	topic_writer_init(rwr, r2);

	char msg[16];
	sprintf(msg, "msg %d", 1);
	topic_writer_write(wr, msg, 16);
	sprintf(msg, "msg %d", 2);
	topic_writer_write(wr, msg, 16);
	sprintf(msg, "msg %d", 3);
	topic_writer_write(wr, msg, 16);
	topic_ring_print_details(r1);
	topic_ring_print_details(r2);

	topic_ring_copy(rrd, rwr);
	topic_ring_print_details(r2);

	bool succ;
	succ = topic_reader_read(rd, test_rx.arr, 32);
	info("succ %d: ", succ);
	topic_reader_print(rd);
	succ = topic_reader_read(rd, test_rx.arr, 32);
	info("succ %d: ", succ);
	topic_reader_print(rd);
	succ = topic_reader_read(rd, test_rx.arr, 32);
	info("succ %d: ", succ);
	topic_reader_print(rd);
	succ = topic_reader_read(rd, test_rx.arr, 32);
	info("succ %d: ", succ);
	topic_reader_print(rd);

	size_t i;
	for (i = 0; i < 12; i++)
	{
		sprintf(msg, "msg %lu", i + 10);
		topic_writer_write(wr, msg, 16);
	}
	topic_ring_copy(rrd, rwr);
	topic_ring_print_details(r2);
}

/*
 * [
 *   p1 : partition := [
 *     a: publish topic t1,
 *     b: subscribe topic t1,
 *   }
 *   p2 : partition := [
 *     c: publish topic t1,
 *     d: subscribe topic t1,
 *   }
 * ]
 */
static struct thinros_master_t test_master;
static struct node_handle_t test_node_c, test_node_d;
static struct publisher_t test_node_a_pub, test_node_c_pub;
static struct subscriber_t test_node_b_sub, test_node_d_sub;

static void test_thinros_master(void)
{
	struct thinros_master_t *m = &test_master;
	struct topic_partition_t *p1 = &this_part;
	struct topic_partition_t *p2 = &other_part;

	struct node_handle_t *a = &test_node_a;
	struct node_handle_t *b = &test_node_b;
	struct node_handle_t *c = &test_node_c;
	struct node_handle_t *d = &test_node_d;

	struct publisher_t *pub_a = &test_node_a_pub;
	struct publisher_t *pub_c = &test_node_c_pub;
	struct subscriber_t *sub_b = &test_node_b_sub;
	struct subscriber_t *sub_d = &test_node_d_sub;

	topic_partition_init(p1);
	topic_partition_init(p2);

	thinros_node(a, p1, "a");
	thinros_node(b, p1, "b");
	thinros_node(c, p2, "c");
	thinros_node(d, p2, "d");

	thinros_advertise(pub_a, a, "fwd_scan");
	thinros_advertise(pub_c, c, "fwd_scan");
	thinros_subscribe(sub_b, b, "fwd_scan", test_dummy_callback);
	thinros_subscribe(sub_d, d, "fwd_scan", test_dummy_callback);

	thinros_master_init(m);
	thinros_master_add(m, p1);
	thinros_master_add(m, p2);
	thinros_master_build(m);
	thinros_master_print(m);

	thinros_master_switch_to(m, 0);
	thinros_publish(pub_a, "msg 1", 16);
	thinros_publish(pub_a, "msg 2", 16);
	thinros_publish(pub_a, "msg 3", 16);
	thinros_spin(b, SPIN_ONCE, NULL, 0);

	info("now switch to partition 2\n");
	thinros_master_switch_to(m, 1);
	thinros_publish(pub_c, "msg 4", 16);
	thinros_publish(pub_c, "msg 5", 16);
	thinros_publish(pub_c, "msg 6", 16);
	thinros_spin(d, SPIN_ONCE, NULL, 0);

	info("now switch to partition 1 again\n");
	thinros_master_switch_to(m, 0);
	thinros_spin(b, SPIN_ONCE, NULL, 0);
}

static void test_all(void)
{
	test_topic_ring();
	test_topic_reader_writer();
	test_topic_namespace();
	test_partition_local();
	test_topic_ring_copy();
	test_thinros_master();
}


int main(int argc, char **argv)
{
	test_all();
	return 0;
}

#endif /* UNIT_TEST */
/*-- end of test section --*/
