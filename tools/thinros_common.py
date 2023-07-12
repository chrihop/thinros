#!/usr/bin/env python3

import subprocess
# thinros-common.py
from collections import OrderedDict
from dataclasses import dataclass, field
from typing import List, Tuple
from pathlib import Path
import json


@dataclass
class Topic:
    name: str
    queue_length: int
    c_definition: List[str]
    uuid: int = None
    publish_nodes: List['Node'] = None
    subscribe_nodes: List['Node'] = None
    publish_pars: List[int] = None
    subscribe_pars: List[int] = None
    _global_index: int = 1

    def __post_init__(self):
        self.uuid = Topic._global_index
        Topic._global_index += 1

    @classmethod
    def reset(cls):
        cls._global_index = 1

    def publish_by(self, node: 'Node'):
        if self.publish_nodes is None:
            self.publish_nodes = []
        self.publish_nodes.append(node)
        return self

    def subscribe_by(self, node: 'Node'):
        if self.subscribe_nodes is None:
            self.subscribe_nodes = []
        if node not in self.subscribe_nodes:
            self.subscribe_nodes.append(node)
        return self


@dataclass
class Callback:
    name: str


@dataclass
class Node:
    name: str
    publish: List[Topic] = field(default_factory=list)
    subscribe: List[Tuple[Topic, List[Callback]]] = field(default_factory=list)
    startup: List[Callback] = field(default_factory=list)
    shutdown: List[Callback] = field(default_factory=list)
    spin: List[Callback] = field(default_factory=list)

    def belongs_to(self, partition: 'Partition'):
        self.partition = partition
        return self

    def on_message(self, topic: Topic, callback: Callback):
        for t, callbacks in self.subscribe:
            if t == topic:
                for c in callbacks:
                    if c.name == callback.name:
                        raise ValueError(f'Callback {callback.name} is '
                                         f'defined twice in node {self.name}, '
                                         f'malformed configration!')
                callbacks.append(callback)
                return self
        self.subscribe.append((topic, [callback]))
        return self

    def on_startup(self, callback: Callback):
        for c in self.startup:
            if c.name == callback.name:
                raise ValueError(f'Callback {callback.name} is defined twice '
                                 f'in node {self.name}, malformed '
                                 f'configration!')
        self.startup.append(callback)
        return self

    def on_shutdown(self, callback: Callback):
        for c in self.shutdown:
            if c.name == callback.name:
                raise ValueError(f'Callback {callback.name} is defined twice '
                                 f'in node {self.name}, malformed '
                                 f'configration!')
        self.shutdown.append(callback)
        return self

    def on_spin(self, callback: Callback):
        for c in self.spin:
            if c.name == callback.name:
                raise ValueError(f'Callback {callback.name} is defined twice '
                                 f'in node {self.name}, malformed '
                                 f'configration!')
        self.spin.append(callback)
        return self


@dataclass
class Partition:
    par_id: int
    nodes: List[Node]


class Network:
    def __init__(self, json: dict):
        self.json = json
        self.topics: OrderedDict[str, Topic] = OrderedDict()
        self.nodes: OrderedDict[str, Node] = OrderedDict()
        self.partitions: OrderedDict[int, Partition] = OrderedDict()

    def save(self, path: Path):
        network = OrderedDict({
            '$schema': 'https://github.com/chrihop/thinros/blob/v2/lib/topic.schema.json',
            'trs': '',
            'topics': [],
            'network': OrderedDict({
                'nodes': [],
                'partitions': [],
            }),
        })
        for t in self.topics.values():
            network['topics'].append(OrderedDict({
                'name': t.name,
                'queue_length': t.queue_length,
                'c_definitions': t.c_definition,
            }))

        for n in self.nodes.values():
            node = OrderedDict({
                'name': n.name,
                'publish': [],
                'subscribe': [],
            })
            for t in n.publish:
                node['publish'].append(t.name)
            for t, callbacks in n.subscribe:
                for c in callbacks:
                    node['subscribe'].append(OrderedDict({
                        'topic': t.name,
                        'callback': c.name,
                    }))
            if len(n.startup) > 0:
                node['on_startup'] = []
                for c in n.startup:
                    node['on_startup'].append(c.name)
            if len(n.shutdown) > 0:
                node['on_shutdown'] = []
                for c in n.shutdown:
                    node['on_shutdown'].append(c.name)
            network['network']['nodes'].append(node)

        for p in self.partitions.values():
            partition = OrderedDict({
                'id': p.par_id,
                'nodes': [],
            })
            for n in p.nodes:
                partition['nodes'].append(n.name)
            network['network']['partitions'].append(partition)

        with open(path, 'w') as f:
            json.dump(network, f, indent=2)

    def load_all(self):
        self.load_partitions()
        return self

    def load_topics(self):
        self.topics.clear()
        Topic.reset()
        for t in self.json['topics']:
            name = t['name']
            if name in self.topics:
                raise ValueError(f'Topic {name} is defined twice, malformed '
                                 f'configration!')
            self.topics[name] = Topic(name, t['queue_length'],
                                      t['c_definitions'])
        return self

    def load_nodes(self):
        self.load_topics()
        self.nodes.clear()
        for n in self.json['network']['nodes']:
            name = n['name']
            if name in self.nodes:
                raise ValueError(f'Node {name} is defined twice, malformed '
                                 f'configration!')
            node = Node(name, [], [])
            for t in n['publish']:
                if t not in self.topics:
                    raise ValueError(f'Topic {t} is not defined, malformed '
                                     f'configration!')
                topic = self.topics[t]
                node.publish.append(topic)
                topic.publish_by(node)
            for s in n['subscribe']:
                t, callback = s['topic'], s['callback']
                if t not in self.topics:
                    raise ValueError(
                        f'Topic {t} of node {n} is not defined, malformed '
                        f'configration!')
                topic = self.topics[t]
                node.on_message(topic, Callback(callback))
                topic.subscribe_by(node)
            if 'on_startup' in n:
                for c in n['on_startup']:
                    node.on_startup(Callback(c))
            if 'on_shutdown' in n:
                for c in n['on_shutdown']:
                    node.on_shutdown(Callback(c))
            if 'on_spin' in n:
                for c in n['on_spin']:
                    node.on_spin(Callback(c))
            self.nodes[name] = node
        return self

    def load_partitions(self):
        self.load_nodes()
        self.partitions.clear()
        for p in self.json['network']['partitions']:
            par_id = p['id']
            if par_id in self.partitions:
                raise ValueError(
                    f'Partition {par_id} is defined twice, malformed '
                    f'configration!')
            self.partitions[par_id] = Partition(par_id, [])
            for n in p['nodes']:
                if n not in self.nodes:
                    raise ValueError(f'Node {n} is not defined, malformed '
                                     f'configration!')
                node = self.nodes[n]
                node.belongs_to(self.partitions[par_id])
                self.partitions[par_id].nodes.append(node)
        return self


def get_version():
    try:
        ver = subprocess.check_output(
            ['git', 'rev-parse', '--short', 'HEAD']
        ).strip().decode('utf-8')
    except subprocess.CalledProcessError:
        ver = 'unknown'
    return ver
