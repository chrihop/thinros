#!/usr/bin/env python3

from pathlib import Path
from pprint import pprint
from collections import OrderedDict
import json
from argparse import ArgumentParser
from typing import List, Iterable

import sys, os


sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from tools.thinros_common import Topic, Node, Callback, Partition, Network, \
    get_version


def print_line(file: Path, lineno: int):
    texts = file.open('r').readlines()
    start = max(0, lineno - 5)
    end = min(len(texts), lineno + 5)
    for i in range(start, end):
        if i == lineno:
            print(f'-> {i + 1:3d} {texts[i].rstrip()}')
        else:
            print(f'   {i + 1:3d} {texts[i].rstrip()}')


class Generator(Network):
    def __init__(self, flatten_json: Path):
        self.flatten_json = flatten_json
        self.flatten_rules = json.load(flatten_json.open('r'))
        self.description = Path(self.flatten_rules['flt'])
        network = OrderedDict()
        search_paths = set()
        search_paths.add(Path(__file__).parent)
        search_paths.add(flatten_json.parent)
        search_paths.add(Path(__file__).parent.parent / 'config')

        loaded = set()
        for n in self.flatten_rules['networks']:
            f = n['file']
            found = False
            for p in search_paths:
                if p / f in loaded:
                    print(f'Network description file `{f}` already loaded')
                    continue
                if (p / f).exists():
                    network.update(json.load((p / f).open('r'),
                                             object_pairs_hook=OrderedDict))
                    loaded.add(p / f)
                    found = True
                    break
            if not found:
                msg = f'Network description file `{f}` not found'
                print(msg)
                print_line(self.description, n['lineno'])
                raise FileNotFoundError(msg)
        super().__init__(network)

    def load(self):
        self.load_all()

    def merge(self, into: str, merge: Iterable[str]) -> Node:
        to_merge = [self.nodes[n] for n in merge]
        if not all(n.partition == to_merge[0].partition for n in to_merge):
            raise ValueError(f'Nodes to merge must belong to the same partition')
        par = to_merge[0].partition

        node = Node(name=into)
        for n in to_merge:
            node.publish.extend(n.publish)
            node.subscribe.extend(n.subscribe)
            node.startup.extend(n.startup)
            node.shutdown.extend(n.shutdown)
            node.spin.extend(n.spin)
            par.nodes.remove(n)
        node.belongs_to(to_merge[0].partition)
        par.nodes.append(node)
        return node

    def convert(self) -> Network:
        c_nodes = OrderedDict()

        remaining = set(self.nodes.keys())
        for r in self.flatten_rules['rules']:
            try:
                to_merge = set()
                for n in r['flatten']:
                    if n in remaining:
                        to_merge.add(n)
                    else:
                        raise(ValueError(f'Node `{n}` not exists, or already flattened'))
                merge_into = r['into']
                node: Node = self.merge(merge_into, to_merge)
                c_nodes[merge_into] = node
                remaining -= to_merge
            except ValueError as e:
                print(e)
                print_line(self.description, r['lineno'])
                continue

        for n in remaining:
            c_nodes[n] = self.nodes[n]

        network = Network({})
        network.topics = self.topics.copy()
        network.nodes = c_nodes
        network.partitions = self.partitions.copy()
        return network

def main():
    parser = ArgumentParser(description='Flatten ThinROS network')
    parser.add_argument('-f', '--file', type=Path,
                        help='input flatten json file',
                        required=True)
    parser.add_argument('-o', '--output', type=Path,
                        help='output flattened network json',
                        required=True)
    args = parser.parse_args()
    generator = Generator(args.file)
    generator.load()
    network = generator.convert()
    network.save(args.output)


if __name__ == '__main__':
    main()
