#!/usr/bin/env python3

import json
from collections import OrderedDict
from pathlib import Path
from datetime import datetime
import argparse

from jsonschema import validate
from jinja2 import Environment, FileSystemLoader, select_autoescape
from tqdm import tqdm
from pprint import pprint

import sys, os

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from tools.thinros_common import Topic, Node, Callback, Partition, Network, \
    get_version


class Generator(Network):
    def __init__(self, network_file: Path):
        super().__init__(json.load(network_file.open('r')))
        self.network_file = network_file

    def load(self):
        self.load_all()
        self.validate()
        return self

    def validate(self):
        return self

    def rewrite_node(self, node: Node) -> dict:
        aggregate = {
            'startup': [],
            'shutdown': [],
            'spin': [],
            'subscribe': {},
        }

        if len(node.startup) == 0 or len(node.startup) > 1:
            node.startup_entry = Callback(node.name, 'default')
            aggregate['startup'].extend(node.startup)
        else:
            node.startup_entry = node.startup[0]

        if len(node.shutdown) == 0 or len(node.shutdown) > 1:
            node.shutdown_entry = Callback(node.name, 'default')
            aggregate['shutdown'].extend(node.shutdown)
        else:
            node.shutdown_entry = node.shutdown[0]

        if len(node.spin) == 0 or len(node.spin) > 1:
            node.spin_entry = Callback(node.name, 'default')
            aggregate['spin'].extend(node.spin)
        else:
            node.spin_entry = node.spin[0]

        message_handlers = {}
        for t, callbacks in node.subscribe:
            if t.name not in message_handlers:
                message_handlers[t.name] = []
            message_handlers[t.name].extend(callbacks)

        for t, callbacks in message_handlers.items():
            if len(callbacks) == 0 or len(callbacks) > 1:
                aggregate['subscribe'][t] = {
                    'entry': Callback(node.name, 'default'),
                    'callbacks': callbacks
                }
            else:
                aggregate['subscribe'][t] = {
                    'entry': callbacks[0],
                    'callbacks': []
                }
        return aggregate

    def save_stubs(self, out_dir: Path):
        env = Environment(
            loader=FileSystemLoader(Path(__file__).parent / 'templates'),
            autoescape=select_autoescape(
                enabled_extensions=['jinja2.c', 'jina2.h']
            ),
            trim_blocks=True
        )
        misc = {
            'network': self.network_file.name,
            'date': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            'version': get_version()
        }

        node_h = env.get_template('thinros_object_node_interface.jinja2.h')
        node_c = env.get_template('thinros_object_node_interface.jinja2.c')
        node_impl = env.get_template('thinros_object_node_impl.jinja2.c')

        for name, node in self.nodes.items():
            aggr = self.rewrite_node(node)
            node_h.stream(
                node=node,
                topics=self.topics,
                partitions=self.partitions,
                aggregate=aggr,
                misc=misc,
            ).dump((out_dir / f'{name}.h').open('w'))
            node_c.stream(
                node=node,
                topics=self.topics,
                partitions=self.partitions,
                aggregate=aggr,
                misc=misc,
            ).dump((out_dir / f'{name}.c').open('w'))
            node_impl.stream(
                node=node,
                topics=self.topics,
                partitions=self.partitions,
                aggregate=aggr,
                misc=misc,
            ).dump((out_dir / f'{name}_impl.c').open('w'))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-n', '--network', type=Path, required=True,
                        help='network configuration file')
    parser.add_argument('-o', '--out_dir', type=Path, required=True,
                        help='output directory')
    parser.add_argument('-s', '--stubs', action='store_true',
                        help='generate stubs')
    args = parser.parse_args()

    gen = Generator(args.network)
    gen.load()
    gen.save_stubs(args.out_dir)


if __name__ == '__main__':
    main()
