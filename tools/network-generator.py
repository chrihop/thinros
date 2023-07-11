#!/usr/bin/env python3

import json
import subprocess
from collections import OrderedDict
from dataclasses import dataclass
from pathlib import Path
from typing import List, Tuple
from datetime import datetime
import argparse

import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
import networkx as nx
from networkx.drawing.nx_agraph import graphviz_layout
from jsonschema import validate
from jinja2 import Environment, FileSystemLoader, select_autoescape
from humanfriendly import parse_size
import sys, os

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from tools.thinros_common import Topic, Node, Callback, Partition, Network, \
    get_version


class Generator(Network):
    def __init__(self,  network_json: Path, platform_json: Path):
        super().__init__(json.load(network_json.open('r')))
        schema_dir = Path(__file__).parent / 'schema'
        platform_schema = json.load((schema_dir /'platform.schema.json').open('r'))
        topic_schema = json.load((schema_dir / 'topic.schema.json').open('r'))
        validate(json.load(platform_json.open('r')), platform_schema)
        validate(json.load(network_json.open('r')), topic_schema)

        self.topics_json = network_json
        self.platform_json = platform_json
        self.json = json.load(platform_json.open('r'))
        self.json.update(json.load(network_json.open('r')))
        self.topics: OrderedDict[str, Topic] = OrderedDict()
        self.nodes: OrderedDict[str, Node] = OrderedDict()
        self.partitions: OrderedDict[int, Partition] = OrderedDict()

    def load(self):
        self.load_all()
        self.convert()
        self.calc()
        self.validate()
        return self

    def validate(self):
        for n in self.nodes.values():
            if not hasattr(n, 'partition'):
                raise ValueError(f'Network validation failed: node `{n.name}` is'
                                 f' not assigned to any partition')
        return self

    def calc(self):
        for t in self.topics.values():
            pub_pars = set()
            sub_pars = set()
            pub_pars.update([n.partition.par_id for n in t.publish_nodes])
            sub_pars.update([n.partition.par_id for n in t.subscribe_nodes])
            t.publish_pars = pub_pars
            t.subscribe_pars = sub_pars
        return self

    def convert(self):
        conversion = {
            'nonsecure_partition': ['size'],
            'limits': ['topic-buffer-size', 'max-message-size', 'padding']
        }
        for k, v in conversion.items():
            group = self.json[k]
            for itm in v:
                group[itm] = parse_size(group[itm], binary=True)
        return self

    def save_config_c(self, out_dir: Path):
        env = Environment(
            loader=FileSystemLoader(Path(__file__).parent / 'templates'),
            autoescape=select_autoescape(
                enabled_extensions=['jinja2.c', 'jina2.h']
            ),
            trim_blocks=True
        )
        misc = {
            'topics_json': self.topics_json.name,
            'platform_json': self.platform_json.name,
            'date': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            'version': get_version()
        }

        template = env.get_template('thinros_cfg.jinja2.h')
        template.stream(
            topics=self.topics,
            nodes=self.nodes,
            partitions=self.partitions,
            data=self.json,
            misc=misc,
        ).dump((out_dir / 'thinros_cfg.h').open('w'))

        template = env.get_template('thinros_cfg.jinja2.c')
        template.stream(
            topics=self.topics,
            nodes=self.nodes,
            partitions=self.partitions,
            data=self.json,
            misc=misc,
        ).dump((out_dir / 'thinros_cfg.c').open('w'))

    def to_graph(self):
        g = nx.DiGraph()
        for n in self.topics.values():
            g.add_node(n.name, type='topic')
        for n in self.nodes.values():
            g.add_node(n.name, type='node', partition=n.partition.par_id)
            for t in n.publish:
                g.add_edge(n.name, t.name)
            for t, _ in n.subscribe:
                g.add_edge(t.name, n.name)
        return g

    @staticmethod
    def rgb_to_hex(rgb: Tuple[int, int, int]) -> str:
        rgb = tuple([int(x * 255) for x in rgb])
        return '#%02x%02x%02x' % rgb

    def show(self, out_dir: Path):
        g = self.to_graph()
        pos = graphviz_layout(g, prog='dot')
        nx.draw_networkx_nodes(
            g, pos,
            nodelist=[n for n in g.nodes if g.nodes[n]['type'] == 'topic'],
            node_shape='s', node_color='grey', alpha=0.5)
        cmap = plt.cm.get_cmap('rainbow', len(self.partitions))
        for i, p in enumerate(self.partitions.values()):
            color = self.rgb_to_hex(cmap(i)[:3])
            nodes = [n for n in g.nodes if g.nodes[n]['type'] == 'node' and
                     g.nodes[n]['partition'] == p.par_id]
            x = [pos[n][0] for n in nodes]
            y = [pos[n][1] for n in nodes]
            print(f'Partition {p.par_id}: {len(nodes)} nodes: {x}, {y}')
            llp = min(x) - 40, min(y) - 20
            width = max(x) - min(x) + 80
            height = max(y) - min(y) + 40
            nx.draw_networkx_nodes(
                g, pos, nodelist=nodes,
                node_shape='o', node_color=color, alpha=0.5)
            rect = Rectangle(llp, width, height, fill=False, color=color, alpha=0.4)
            plt.gca().add_patch(rect)
            rect_attr_loc = (llp[0], llp[1] + height + 5)
            plt.text(*rect_attr_loc, f'par #{p.par_id}', fontsize=5, color=color)
        nx.draw_networkx_edges(g, pos, arrows=True, arrowsize=20, alpha=0.6)
        nx.draw_networkx_labels(g, pos, font_size=8)
        plt.margins(0.5)
        plt.savefig(out_dir / 'network.png', dpi=300)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-n', '--network', type=Path, required=True,
                        help='network configuration file')
    parser.add_argument('-p', '--platform', type=Path, required=True,
                        help='platform configuration file')
    parser.add_argument('-o', '--out_dir', type=Path, required=True,
                        help='output directory')
    parser.add_argument('--draw', action='store_true', default=False,
                        help='draw network')
    args = parser.parse_args()

    gen = Generator(args.network, args.platform)
    gen.load()
    gen.save_config_c(args.out_dir)
    if args.draw:
        gen.show(args.out_dir)


if __name__ == '__main__':
    main()
