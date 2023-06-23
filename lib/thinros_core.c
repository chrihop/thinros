#include "thinros_core.h"

/*-- debug functions --*/
void
topic_ring_print(struct topic_ring_t* r)
{
    atomic_size_t head = atomic_load(&r->head);
    info("ring @0x%lx (len %lu elem_sz %lu) head %lu: ", (uintptr_t)r, r->n,
        r->elem_sz, head);
    size_t tail = (head > r->n) > 0 ? (head - r->n) : 0;
    for (size_t i = tail; i < head; i++)
    {
        struct topic_data_t* data = of(r, i % r->n);
        info("< %lu : %s (%lu) >", i,
            atomic_load(&data->status) == TOPIC_READY ? "ready" : "empty", data->timestamp);
    }
    info("\n");
}

void
topic_ring_print_details(struct topic_ring_t* r)
{
    topic_ring_print(r);
    atomic_size_t head = atomic_load(&r->head);
    size_t        tail = (head - r->n) > 0 ? (head - r->n) : 0;
    for (size_t i = tail; i < head; i++)
    {
        struct topic_data_t* data = of(r, i % r->n);
        info("%lu: ", i);
        for (size_t j = 0; j < MIN(r->elem_sz, 18lu); j++)
        {
            info("0x%02x ", data->data[j]);
        }
        if (r->elem_sz > 18)
        {
            info(" ...");
        }
        info("\n");
        for (size_t j = 0; j < MIN(r->elem_sz, 18lu); j++)
        {
            info("   %c ", data->data[j]);
        }
        if (r->elem_sz > 18)
        {
            info(" ...");
        }
        info("\n");
    }
}

void
topic_writer_print(struct topic_writer_t* wr)
{
    info("writer @0x%lx -> ring @0x%lx, working at %lu\n", (uintptr_t)wr,
        (uintptr_t)wr->ring, wr->index);
}

void
topic_reader_print(struct topic_reader_t* rd)
{
    info("reader @0x%lx -> ring @0x%lx, working at %lu, timestamp %lu, "
         "read head %lu tail %lu: ",
        (uintptr_t)rd, (uintptr_t)rd->ring, rd->index, rd->timestamp,
        rd->read_head, rd->read_tail);
    for (size_t i = rd->read_tail; i < rd->read_head; i++)
    {
        size_t idx = i % rd->ring->n;
        info("< %lu: %s >", i,
            rd->read_ring[idx] == READER_NOT_READ ? "not read" : "has read");
    }
    info("\n");
}

void
thinros_subscriber_print(struct subscriber_t* s)
{
    info("subscriber: 0x%lx, callback: 0x%lx ", (size_t)s, (size_t)s->callback);
    topic_reader_print(&s->local_reader);
    topic_reader_print(&s->external_reader);
}

void
thinros_publisher_print(struct publisher_t* p)
{
    info("publisher: 0x%lx, ", (size_t)p);
    topic_writer_print(&p->writer);
}

void
thinros_node_print(struct node_handle_t* n)
{
    info("node `%s` partition 0x%lx, subscribing (%lu): ", n->node_name,
        (size_t)n->par, n->n_subscribers);
    for (size_t i = 0; i < n->n_subscribers; i++)
    {
        struct subscriber_t*           s = n->subscribers[i];
        struct topic_namespace_item_t* ns
            = topic_namespace_query_by_uuid(s->topic_uuid);
        info("on topic `%s` (uuid: %lu, local 0x%lx external 0x%lx), ",
            ns->name, s->topic_uuid, (size_t)s->local_reader.ring,
            (size_t)s->external_reader.ring);
    }
    info("\n");
}

void
topic_partition_print(struct topic_partition_t* par)
{
    info("partition 0x%lx id %lu topics:\n", (size_t)par, par->partition_id);
    for (size_t i = 0; i < par->registry.n; i++)
    {
        struct topic_registry_item_t* t = &par->registry.topic[i];
        info("  [uuid %8lu %c %c local 0x%lx external 0x%lx]\n", t->uuid,
            t->to_publish ? 'P' : ' ', t->to_subscribe ? 'S' : ' ',
            (size_t)t->local_ring, (size_t)t->external_ring);
    }
}

void
thinros_master_print(struct thinros_master_t* m)
{
    info("partitions (%lu):\n", m->n_partitions);
    for (size_t i = 0; i < m->n_partitions; i++)
    {
        struct master_record_t* par = &m->partitions[i];
        info("<par %lu:", i);
        topic_partition_print(par->address);
        info("==\n");
        info("replicators (%lu):\n", par->n_replicators);
        for (size_t j = 0; j < par->n_replicators; j++)
        {
            struct topic_replicator_t* rep = &par->replicators[j];
            info("  topic uuid %lu -> ring @0x%lx, from:\n", rep->topic_uuid,
                (size_t)rep->destination.ring);
            for (size_t k = 0; k < rep->n_sources; k++)
            {
                struct topic_reader_t* src = &rep->sources[k];
                info("    ring 0x%lx\n", (size_t)src->ring);
            }
        }
    }
}

/*-- end of debug functions --*/

void
topic_ring_init(struct topic_ring_t* p_ring, size_t n, size_t elem_sz)
{
    ASSERT(p_ring != NULL);
    ASSERT(((uintptr_t)p_ring) % 8 == 0); /* ensure ring is 8-byte aligned */
    ASSERT(n <= MAX_RING_ELEMS);

    size_t               i;
    struct topic_ring_t* r = p_ring;
    atomic_store(&r->head, 0);
    r->n       = n;
    r->elem_sz = elem_sz + sizeof(struct topic_data_t);
    for (i = 0; i < n; i++)
    {
        struct topic_data_t* data = of(r, i);
        atomic_init(&data->status, TOPIC_EMPTY);
    }
}

size_t
topic_ring_alloc(struct topic_ring_t* r)
{
    ASSERT(r != NULL);
    ASSERT(r->n != 0 && "ring is not initialized.");

    atomic_size_t        loc  = atomic_fetch_add(&r->head, 1);
    size_t               idx  = loc % r->n;
    struct topic_data_t* data = of(r, idx);
    atomic_init(&data->status, TOPIC_EMPTY);
    data->timestamp = loc;
    return idx;
}

static void
topic_ring_mk_ready(struct topic_ring_t* r, size_t idx)
{
    ASSERT(idx < r->n);

    struct topic_data_t* data = of(r, idx);
    atomic_store(&data->status, TOPIC_READY);
}

void
topic_writer_init(struct topic_writer_t* w, struct topic_ring_t* r)
{
    w->ring  = r;
    w->index = 0;
}

struct topic_data_t*
topic_writer_next_avail(struct topic_writer_t* w)
{
    ASSERT(w != NULL);
    ASSERT(w->ring != NULL);

    w->index                  = topic_ring_alloc(w->ring);
    struct topic_data_t* data = of(w->ring, w->index);
    return data;
}

void
topic_writer_complete(struct topic_writer_t* w)
{
    ASSERT(w != NULL);
    ASSERT(w->ring != NULL);

    topic_ring_mk_ready(w->ring, w->index);
}

struct topic_data_t*
topic_writer_write(struct topic_writer_t* w, void* src, size_t sz)
{
    ASSERT(w != NULL);
    ASSERT(sz <= w->ring->elem_sz - sizeof(struct topic_data_t));

    struct topic_data_t* data = topic_writer_next_avail(w);
    memcpy(data->data, src, sz);
    topic_writer_complete(w);
    return data;
}

void
topic_reader_init(struct topic_reader_t* rd, struct topic_ring_t* r)
{
    rd->read_head = 0;
    rd->read_tail = 0;
    rd->index     = 0;
    rd->timestamp = 0;
    rd->ring      = r;
}

static void
topic_reader_sync(struct topic_reader_t* rd)
{
    ASSERT(rd != NULL);
    ASSERT(rd->ring != NULL);

    atomic_size_t hd = atomic_load(&rd->ring->head);
    size_t        n  = rd->ring->n;

    size_t tl     = hd > n ? (hd - n) : 0;
    /* fast-forward: skip overwritten elements */
    rd->read_head = rd->read_head > tl ? rd->read_head : tl;
    rd->read_tail = rd->read_tail > tl ? rd->read_tail : tl;
    /* sync read_head */
    for (size_t i = rd->read_head; i < hd; i++)
    {
        rd->read_ring[i % n] = READER_NOT_READ;
    }
    rd->read_head = hd;
}

/**
 * read exactly the next slot, no skip of elements that are not ready
 *
 * @param rd
 * @return
 */
struct topic_data_t*
topic_reader_read_next(struct topic_reader_t* rd)
{
    topic_reader_sync(rd);
    if (rd->read_head == rd->read_tail)
    {
        /* nothing to read */
        return NULL;
    }
    size_t idx;
    idx                       = rd->read_tail % rd->ring->n;
    struct topic_data_t* data = of(rd->ring, idx);
    if (atomic_load(&data->status) == TOPIC_EMPTY)
    {
        /* topic not ready to read */
        return NULL;
    }
    rd->index     = idx;
    rd->timestamp = data->timestamp;
    return data;
}

/**
 * read as far as possible, skip elements that are not ready
 *
 * @param rd
 * @return
 */
struct topic_data_t*
topic_reader_read_eager(struct topic_reader_t* rd)
{
    /* read any element that is ready */
    topic_reader_sync(rd);
    struct topic_data_t* data = NULL;

    for (size_t i = rd->read_tail; i < rd->read_head; i++)
    {
        size_t               idx = i % rd->ring->n;
        struct topic_data_t* cur = of(rd->ring, idx);
        if (atomic_load(&cur->status) == TOPIC_READY && rd->read_ring[idx] == READER_NOT_READ)
        {
            rd->index     = idx;
            rd->timestamp = cur->timestamp;
            data          = cur;
            break;
        }
    }
    return data;
}

bool
topic_reader_complete(struct topic_reader_t* rd)
{
    size_t               i;
    size_t               n    = rd->ring->n;
    struct topic_data_t* data = of(rd->ring, rd->index);
    /* check if the data has been overwritten during the reading */
    bool                 consistent
        = (atomic_load(&data->status) == TOPIC_READY && data->timestamp == rd->timestamp);
    rd->read_ring[rd->index] = READER_HAS_READ;
    for (i = rd->read_tail; i < rd->read_head; i++)
    {
        if (rd->read_ring[i % n] == READER_NOT_READ)
        {
            break;
        }
    }
    /* now i = next to ready elem or head */
    rd->read_tail = i;

    return consistent;
}

/**
 * read all available messages
 *
 * @param rd
 * @param buffer (temporary) should be larger than one message size
 * @param callback
 * @return
 */
size_t
topic_reader_read_all(struct topic_reader_t* rd, void* buffer, size_t sz,
    thinros_callback_on_t callback)
{
    ASSERT(callback != NULL);

    topic_reader_sync(rd);
    size_t total_read = 0;

    for (size_t i = rd->read_tail; i < rd->read_head; i++)
    {
        size_t               idx = i % rd->ring->n;
        struct topic_data_t* cur = of(rd->ring, idx);
        if (atomic_load(&cur->status) == TOPIC_READY && rd->read_ring[idx] == READER_NOT_READ)
        {
            rd->index     = idx;
            rd->timestamp = cur->timestamp;
            ASSERT(rd->ring->elem_sz < sz);
            memcpy(buffer, cur->data, MIN(sz, rd->ring->elem_sz));
            bool succ = topic_reader_complete(rd);
            if (succ)
            {
                callback(buffer);
                total_read++;
            }
            else
            {
                WARN("message %lu in topic ring 0x%lx dropped due to "
                     "overwrite.\n",
                    i, (size_t)rd->ring);
            }
        }
    }
    return total_read;
}

size_t
topic_ring_copy(struct topic_reader_t* rd, struct topic_writer_t* wr)
{
    ASSERT(rd != NULL);
    ASSERT(wr != NULL);
    ASSERT(rd->ring != wr->ring);
    ASSERT(rd->ring->elem_sz == wr->ring->elem_sz);

    topic_reader_sync(rd);
    size_t copied = 0;
    size_t sz     = wr->ring->elem_sz;

    for (size_t i = rd->read_tail; i < rd->read_head; i++)
    {
        size_t               idx = i % rd->ring->n;
        struct topic_data_t* src = of(rd->ring, idx);
        if (atomic_load(&src->status) == TOPIC_READY && rd->read_ring[idx] == READER_NOT_READ)
        {
            rd->index                = idx;
            rd->timestamp            = src->timestamp;
            struct topic_data_t* dst = topic_writer_next_avail(wr);
            memcpy(dst->data, src->data, sz);
            bool succ = topic_reader_complete(rd);
            if (succ)
            {
                topic_writer_complete(wr);
                copied++;
            }
            else
            {
                WARN("message %lu in topic ring 0x%lx copy failed due to "
                     "overwrite.\n",
                    i, (size_t)rd->ring);
            }
        }
    }

    return (copied);
}

bool
topic_reader_read(struct topic_reader_t* rd, void* dest, size_t sz)
{
    ASSERT(rd != NULL);
    ASSERT(rd->ring != NULL);
    ASSERT(dest != NULL);
    ASSERT(sz <= rd->ring->elem_sz - sizeof(struct topic_data_t));

    bool                 succ;
    struct topic_data_t* data = topic_reader_read_eager(rd);
    if (data == NULL)
    {
        /* nothing to read */
        return false;
    }
    ASSERT(atomic_load(&data->status) == TOPIC_READY);
    memcpy(dest, data->data, sz);
    succ = topic_reader_complete(rd);

    return (succ);
}

void
linear_allocator_init(struct linear_allocator_t* la, size_t total_sz)
{
    la->size = total_sz;
    atomic_store(&la->brk, 0);
}

relative_addr_t
linear_allocator_alloc(struct linear_allocator_t* la, size_t sz)
{
    /* ensure we only allocate on 8-byte aligned addresses */
    size_t align_off = sz % 8;
    if (align_off != 0)
    {
        sz += 8 - align_off;
    }

    ASSERT(sz < la->size - la->brk);

    atomic_size_t loc = atomic_fetch_add(&la->brk, sz);
    return (loc);
}

static void
linear_allocator_used(struct linear_allocator_t* la)
{
    info("used %lu (%lu %%)\n", la->brk, la->brk * 100 / la->size);
}

struct topic_namespace_item_t*
topic_namespace_query_by_name(char* name)
{
    size_t                         i;
    struct topic_namespace_item_t* ns = NULL;
    for (i = 0; i < topic_namespace.n; i++)
    {
        if (strncmp(name, topic_namespace.topic[i].name, TOPIC_NAME_SIZE) == 0)
        {
            ns = &topic_namespace.topic[i];
            break;
        }
    }
    return ns;
}

struct topic_namespace_item_t*
topic_namespace_query_by_uuid(size_t uuid)
{
    size_t                         i;
    struct topic_namespace_item_t* ns = NULL;
    for (i = 0; i < topic_namespace.n; i++)
    {
        if (topic_namespace.topic[i].uuid == uuid)
        {
            ns = &topic_namespace.topic[i];
            break;
        }
    }
    return ns;
}

static void
topic_registry_init(struct topic_registry_t* reg)
{
    reg->n = 0;
}

static struct topic_registry_item_t*
topic_registry_insert(struct topic_registry_t* reg, size_t uuid,
    relative_addr_t local_ring, relative_addr_t external_ring)
{
    ASSERT(reg->n < MAX_TOPICS);
    size_t idx = atomic_fetch_add(&reg->n, 1);
    ASSERT(idx < MAX_TOPICS && "too many topics!");

    reg->topic[idx].uuid          = uuid;
    reg->topic[idx].local_ring    = local_ring;
    reg->topic[idx].external_ring = external_ring;
    reg->topic[idx].to_publish    = FALSE;
    reg->topic[idx].to_subscribe  = FALSE;
    return &reg->topic[idx];
}

struct topic_registry_item_t*
topic_registry_query(struct topic_registry_t* reg, size_t uuid)
{
    ASSERT(reg != NULL);
    struct topic_registry_item_t* item = NULL;
    size_t                        i;
    size_t                        n = reg->n < MAX_TOPICS ? reg->n : MAX_TOPICS;
    for (i = 0; i < n; i++)
    {
        if (reg->topic[i].uuid == uuid)
        {
            item = &reg->topic[i];
            break;
        }
    }

    return item;
}

void
topic_partition_init(struct topic_partition_t* par)
{
    topic_registry_init(&par->registry);
    linear_allocator_init(&par->allocator, TOPIC_BUFFER_SIZE);
    atomic_init(&par->status, PARTITION_INITIALIZED);
}

static uintptr_t
topic_partition_get_addr(struct topic_partition_t* par, relative_addr_t ra)
{
    ASSERT(ra < TOPIC_BUFFER_SIZE);
    return (uintptr_t)(&par->topic_buffer[ra]);
}

static relative_addr_t
topic_partition_to_relative(struct topic_partition_t* par, uintptr_t addr)
{
    ASSERT((uintptr_t)par->topic_buffer <= addr);
    ASSERT(addr < (uintptr_t)par + sizeof(struct topic_partition_t));
    return (relative_addr_t)(addr - (uintptr_t)&par->topic_buffer);
}

void
topic_nonsecure_partition_init(struct topic_partition_t* par)
{
    linear_allocator_init(&par->allocator, TOPIC_BUFFER_SIZE);
}

static struct topic_registry_item_t*
topic_partition_register(
    struct topic_partition_t* par, struct topic_namespace_item_t* ns)
{
    ASSERT(par != NULL);
    ASSERT(ns != NULL);
    relative_addr_t      ra_local_ring, ra_ext_ring;
    struct topic_ring_t *lr, *er;

    size_t sz = sizeof(struct topic_ring_t)
              + ((sizeof(struct topic_data_t) + ns->elem_sz) * ns->length)
              + PADDING_BYTES;

    ra_local_ring = linear_allocator_alloc(&par->allocator, sz);
    ra_ext_ring   = linear_allocator_alloc(&par->allocator, sz);

    lr = (struct topic_ring_t*)topic_partition_get_addr(par, ra_local_ring);
    er = (struct topic_ring_t*)topic_partition_get_addr(par, ra_ext_ring);

    topic_ring_init(lr, ns->length, ns->elem_sz);
    topic_ring_init(er, ns->length, ns->elem_sz);
    struct topic_registry_item_t* topic = topic_registry_insert(
        &par->registry, ns->uuid, ra_local_ring, ra_ext_ring);

    return topic;
}

struct topic_registry_item_t*
topic_partition_get(struct topic_partition_t* par, size_t uuid)
{
    ASSERT(par != NULL);

    struct topic_registry_item_t* topic = NULL;
    topic = topic_registry_query(&par->registry, uuid);
    if (topic != NULL)
    {
        return topic;
    }
    /* not found, create one */
    struct topic_namespace_item_t* ns = topic_namespace_query_by_uuid(uuid);
    if (ns == NULL)
    {
        /* no such topic */
        WARN("topic [uuid=%lu] does not define.\n", uuid);
        return NULL;
    }
    topic = topic_partition_register(par, ns);
    return topic;
}

struct topic_registry_item_t*
topic_partition_get_by_name(struct topic_partition_t* par, char* topic_name)
{
    struct topic_namespace_item_t* ns
        = topic_namespace_query_by_name(topic_name);
    ASSERT(ns != NULL && "topic name not found!");
    struct topic_registry_item_t* topic = topic_partition_get(par, ns->uuid);
    ASSERT(topic != NULL && "cannot allocate topic ring locally!");

    return topic;
}

void
thinros_node(
    struct node_handle_t* n, struct topic_partition_t* par, char* node_name)
{
    n->par     = par;
    size_t len = strnlen(node_name, NODE_NAME_SIZE);
    memcpy(n->node_name, node_name, MIN(len, NODE_NAME_SIZE));
    n->n_subscribers = 0;
}

struct topic_ring_t*
get_local_ring(
    struct topic_partition_t* par, struct topic_registry_item_t* topic)
{
    struct topic_ring_t* r;
    r = (struct topic_ring_t*)topic_partition_get_addr(par, topic->local_ring);
    ASSERT((void*)&par->topic_buffer <= (void*)r
           && (void*)r < (void*)&par->topic_buffer[TOPIC_BUFFER_SIZE]);
    return (r);
}

struct topic_ring_t*
get_external_ring(
    struct topic_partition_t* par, struct topic_registry_item_t* topic)
{
    struct topic_ring_t* r;
    r = (struct topic_ring_t*)topic_partition_get_addr(
        par, topic->external_ring);
    ASSERT((void*)&par->topic_buffer <= (void*)r
           && (void*)r < (void*)&par->topic_buffer[TOPIC_BUFFER_SIZE]);
    return (r);
}

void
thinros_advertise(_out struct publisher_t* publisher,
    _in _in struct node_handle_t* n, _in char* topic_name)
{
    ASSERT(publisher != NULL);
    ASSERT(n != NULL);
    ASSERT(topic_name != NULL);

    struct topic_registry_item_t* topic
        = topic_partition_get_by_name(n->par, topic_name);
    topic->to_publish          = TRUE;
    struct topic_ring_t* local = get_local_ring(n->par, topic);
    topic_writer_init(&publisher->writer, local);
    publisher->topic_uuid = topic_namespace_query_by_name(topic_name)->uuid;
    publisher->partition  = n->par;
}

void
thinros_publish(
    _in struct publisher_t* publisher, _in void* message, _in size_t sz)
{
    ASSERT(publisher != NULL);
    ASSERT(message != NULL);

    topic_writer_write(&publisher->writer, message, sz);
}

static void
node_handle_register_subscriber(struct node_handle_t* n, struct subscriber_t* s)
{
    size_t idx = atomic_fetch_add(&n->n_subscribers, 1);
    ASSERT(idx < MAX_TOPICS_SUBSCRIBE);
    n->subscribers[idx] = s;
}

void
thinros_subscribe(_in struct subscriber_t* subscriber,
    _in struct node_handle_t* n, _in char* topic_name,
    _in thinros_callback_on_t callback)
{
    ASSERT(subscriber != NULL);
    ASSERT(n != NULL);
    ASSERT(topic_name != NULL);
    ASSERT(callback != NULL);

    struct topic_registry_item_t* topic
        = topic_partition_get_by_name(n->par, topic_name);
    topic->to_subscribe = TRUE;

    struct topic_ring_t *local, *external;
    local    = get_local_ring(n->par, topic);
    external = get_external_ring(n->par, topic);
    topic_reader_init(&subscriber->local_reader, local);
    topic_reader_init(&subscriber->external_reader, external);
    subscriber->callback = callback;
    node_handle_register_subscriber(n, subscriber);
    subscriber->topic_uuid = topic_namespace_query_by_name(topic_name)->uuid;
}

static size_t
thinros_spin_once(_in struct node_handle_t* n)
{
    ASSERT(n != NULL);
    size_t total_handled = 0;
    size_t i;

    for (i = 0; i < n->n_subscribers; i++)
    {
        struct subscriber_t* s = n->subscribers[i];
        ASSERT(s != NULL);
        ASSERT(s->callback != NULL);
        total_handled += topic_reader_read_all(
            &s->external_reader, n->buffer, MAX_MESSAGE_SIZE, s->callback);
        total_handled += topic_reader_read_all(
            &s->local_reader, n->buffer, MAX_MESSAGE_SIZE, s->callback);
    }
    return total_handled;
}

static void
thinros_spin_forever(_in struct node_handle_t* n)
{
    while (atomic_load(&n->running) == true)
    {
        thinros_spin_once(n);
    }
}

static void
thinros_spin_yield(_in struct node_handle_t* n, void (*yield)(void))
{
    while (atomic_load(&n->running) == true)
    {
        thinros_spin_once(n);
        yield();
    }
}

static void
thinros_spin_timeout(_in struct node_handle_t* n, uint64_t nanoseconds)
{
    unsigned long long start = time_ns();
    while (atomic_load(&n->running) == true)
    {
        thinros_spin_once(n);
        if (time_ns() - start > nanoseconds)
        {
            break;
        }
    }
}

void
thinros_spin(_in struct node_handle_t* n, enum thinros_spin_type_t type,
    void (*yield)(void), uint64_t                                  nanoseconds)
{
    atomic_store(&n->running, true);

    switch (type)
    {
    case SPIN_ONCE: thinros_spin_once(n); break;
    case SPIN_FOREVER: thinros_spin_forever(n); break;
    case SPIN_YIELD: thinros_spin_yield(n, yield); break;
    case SPIN_TIMEOUT: thinros_spin_timeout(n, nanoseconds); break;
    default: PANIC("unknown spin type!");
    }
}

static void
topic_replicator_init(struct topic_replicator_t* rep, size_t uuid,
    struct topic_ring_t* external_ring)
{
    ASSERT(rep != NULL);
    ASSERT(external_ring != NULL);

    rep->topic_uuid = uuid;
    rep->n_sources  = 0;
    topic_writer_init(&rep->destination, external_ring);
}

static void
topic_replicator_connect(
    struct topic_replicator_t* rep, struct topic_ring_t* local_ring)
{
    size_t idx = atomic_fetch_add(&rep->n_sources, 1);
    ASSERT(idx < MAX_PARTITIONS);
    struct topic_reader_t* rd = &rep->sources[idx];
    topic_reader_init(rd, local_ring);
}

static void
topic_replicator_replicate(struct topic_replicator_t* rep)
{
    size_t i;
    for (i = 0; i < rep->n_sources; i++)
    {
        struct topic_reader_t* rd = &rep->sources[i];
        topic_ring_copy(rd, &rep->destination);
    }
}

void
thinros_master_init(struct thinros_master_t* m)
{
    m->n_partitions = 0;
}

void
thinros_master_add(struct thinros_master_t* m, struct topic_partition_t* par)
{
    size_t idx = atomic_fetch_add(&m->n_partitions, 1);
    ASSERT(idx < MAX_PARTITIONS);
    par->partition_id         = idx;
    struct master_record_t* r = &m->partitions[idx];
    r->address                = par;
    r->n_replicators          = 0;
}

static void
thinros_master_connect_topics_per_partition(
    struct topic_replicator_t* rep, struct topic_partition_t* par)
{
    size_t                   i;
    struct topic_registry_t* topics = &par->registry;
    for (i = 0; i < topics->n; i++)
    {
        struct topic_registry_item_t* topic = &topics->topic[i];
        if (topic->uuid == rep->topic_uuid && topic->to_publish == TRUE)
        {
            struct topic_ring_t* local = get_local_ring(par, topic);
            topic_replicator_connect(rep, local);
        }
    }
}

static void
thinros_master_connect_topics(struct topic_replicator_t* rep,
    size_t current_partition_idx, struct thinros_master_t* m)
{
    size_t i;
    for (i = 0; i < m->n_partitions; i++)
    {
        if (i == current_partition_idx)
        {
            continue;
        }
        thinros_master_connect_topics_per_partition(
            rep, m->partitions[i].address);
    }
}

void
thinros_master_build(struct thinros_master_t* m)
{
    size_t i;
    for (i = 0; i < m->n_partitions; i++)
    {
        size_t j;

        struct master_record_t* par = &m->partitions[i];
        par->n_replicators          = 0;
        for (j = 0; j < par->address->registry.n; j++)
        {
            struct topic_registry_item_t* topic
                = &par->address->registry.topic[j];
            if (topic->to_subscribe == FALSE)
            {
                /* only publish this topic, ignore */
                continue;
            }

            /* subscribe, requires a replicator */
            struct topic_replicator_t* rep
                = &par->replicators[par->n_replicators++];
            struct topic_ring_t* external_ring
                = get_external_ring(par->address, topic);
            topic_replicator_init(rep, topic->uuid, external_ring);
            thinros_master_connect_topics(rep, i, m);
        }
    }
}

void
thinros_master_switch_to(struct thinros_master_t* m, size_t partition_idx)
{
    size_t i;

    struct master_record_t* par = &m->partitions[partition_idx];
    for (i = 0; i < par->n_replicators; i++)
    {
        struct topic_replicator_t* rep = &par->replicators[i];
        topic_replicator_replicate(rep);
    }
}
