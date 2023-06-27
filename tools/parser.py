#!/usr/bin/env python3

from pathlib import Path
from pprint import pprint
from collections import OrderedDict
import json
import io
import argparse

import ply.lex as lex
import ply.yacc as yacc
import jsonschema

tokens = (
    'TOPIC', 'NODE', 'PARTITION', 'MISC',
    'PUBLISH', 'SUBSCRIBE',
    'STATEMENT', 'IDENTIFIER', 'NUMBER', 'STRING', 'COMMENT',
    'LPAREN', 'RPAREN', 'LBRACE', 'RBRACE', 'LBRACKET', 'RBRACKET',
    'COMMA', 'EQUALS', 'SEMICOLON', 'COLON'
)

t_ignore = ' \t\r'


def t_COMMENT(t):
    r'\#.*'
    pass


def t_TOPIC(t):
    r'topic'
    return t


def t_NODE(t):
    r'node'
    return t


def t_PARTITION(t):
    r'partition'
    return t


def t_MISC(t):
    r'misc'
    return t


def t_PUBLISH(t):
    r'publish'
    return t


def t_SUBSCRIBE(t):
    r'subscribe'
    return t


def t_IDENTIFIER(t):
    r'[a-zA-Z_][a-zA-Z0-9_]*'
    return t


def t_NUMBER(t):
    r'((0[xX][a-fA-F0-9]+)|(\b0?[0-9]+)|0[bB][01]+)[uUlL]*'
    return t


def t_STRING(t):
    r'\"([^\\\n]|(\\.))*?\"'
    return t


def t_LPAREN(t):
    r'\('
    return t


def t_RPAREN(t):
    r'\)'
    return t


def t_LBRACE(t):
    r'\{'
    return t


def t_RBRACE(t):
    r'\}'
    return t


def t_LBRACKET(t):
    r'\['
    return t


def t_RBRACKET(t):
    r'\]'
    return t


def t_COMMA(t):
    r','
    return t


def t_EQUALS(t):
    r'='
    return t


def t_SEMICOLON(t):
    r';'
    return t


def t_COLON(t):
    r':'
    return t


def t_newline(t):
    r'\n+'
    t.lexer.lineno += len(t.value)


def t_error(t):
    print("Illegal character '%s'" % t.value[0])
    t.lexer.skip(1)


lexer = lex.lex()


def p_program(p):
    """
    program : program block
            | block
    """
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = p[1] + [p[2]]


def p_block(p):
    """
    block : topic_block
          | node_block
          | partition_block
          | misc_block
    """
    p[0] = p[1]


def p_topic_block(p):
    """
    topic_block : TOPIC IDENTIFIER LPAREN NUMBER RPAREN LBRACE topic_body RBRACE
    """
    p[0] = {
        'type': 'topic',
        'name': p[2],
        'queue_length': p[4],
        'body': p[7]
    }


def p_topic_body(p):
    """
    topic_body : topic_body topic_body_item
                | topic_body_item
    """
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = p[1] + [p[2]]


def p_topic_body_item(p):
    """
    topic_body_item : topic_item_type IDENTIFIER SEMICOLON
                    | topic_item_type IDENTIFIER LBRACKET NUMBER RBRACKET SEMICOLON
    """
    p[0] = ' '.join(p[1:])


def p_topic_item_type(p):
    """
    topic_item_type : topic_item_type IDENTIFIER
                    | IDENTIFIER
    """
    if len(p) == 2:
        p[0] = p[1]
    else:
        p[0] = p[1] + ' ' + p[2]


def p_node_block(p):
    """
    node_block : NODE IDENTIFIER node_body
    """
    p[0] = {
        'type': 'node',
        'name': p[2],
        'body': p[3]
    }


def p_node_body(p):
    """
    node_body : LBRACE publish_block subscribe_block RBRACE
              | LBRACE publish_block RBRACE
              | LBRACE subscribe_block RBRACE
              | LBRACE RBRACE
    """
    if len(p) == 5:
        p[0] = {'publish': p[2]['body'], 'subscribe': p[3]['body']}
    elif len(p) == 4 and p[2]['type'] == 'publish-list':
        p[0] = {'publish': p[2]['body'], 'subscribe': []}
    elif len(p) == 4 and p[2]['type'] == 'subscribe-list':
        p[0] = {'publish': [], 'subscribe': p[2]['body']}
    else:
        p[0] = {'publish': [], 'subscribe': []}


def p_publish_block(p):
    """
    publish_block : PUBLISH EQUALS LBRACE action_body RBRACE
    """
    p[0] = {
        'type': 'publish-list',
        'body': p[4]
    }


def p_subscribe_block(p):
    """
    subscribe_block : SUBSCRIBE EQUALS LBRACE action_body RBRACE
    """
    p[0] = {
        'type': 'subscribe-list',
        'body': p[4]
    }


def p_action_body(p):
    """
    action_body : action_body action_body_item
                | action_body_item
    """
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = p[1] + [p[2]]


def p_action_body_item(p):
    """
    action_body_item : IDENTIFIER
                     | COMMA IDENTIFIER
    """
    if len(p) == 2:
        p[0] = p[1]
    else:
        p[0] = p[2]


def p_partition_block(p):
    """
    partition_block : PARTITION IDENTIFIER LPAREN NUMBER RPAREN LBRACE partition_body RBRACE
    """
    p[0] = {
        'type': 'partition',
        'name': p[2],
        'partition_id': p[4],
        'body': p[7]
    }


def p_partition_body(p):
    """
    partition_body : partition_body partition_body_item
                   | partition_body_item
    """
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = p[1] + [p[2]]


def p_partition_body_item(p):
    """
    partition_body_item : NODE IDENTIFIER SEMICOLON
    """
    p[0] = p[2]


def p_misc_block(p):
    """
    misc_block : MISC LBRACE misc_body RBRACE
    """
    p[0] = {
        'type': 'misc',
        'body': p[3]
    }


def p_misc_body(p):
    """
    misc_body : misc_body misc_body_item
              | misc_body_item
    """
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = p[1] + [p[2]]


def p_misc_body_item(p):
    """
    misc_body_item : IDENTIFIER EQUALS misc_expression SEMICOLON
    """
    p[0] = {
        'type': 'misc-item',
        'name': p[1],
        'value': p[3]
    }


def p_misc_expression(p):
    """
    misc_expression : IDENTIFIER
                    | NUMBER
                    | STRING
    """
    p[0] = p[1]


def p_error(p):
    if p:
        print(f'Syntax error at `{p.value}` type: {p.type} line: {p.lineno}'
              f' {p.lexpos}')
    else:
        print('Syntax error at EOF')


parser = yacc.yacc()


def code_generator(ast, output: Path):
    network = OrderedDict()
    network['$schema'] = "https://github.com/chrihop/thinros/blob/v2/lib/topic.schema.json",
    network['topics'] = []
    network['network'] = OrderedDict({
        'nodes': [],
        'partitions': []
    })
    for o in ast:
        if o['type'] == 'topic':
            topic = OrderedDict({
                'name': o['name'],
                'queue_length': int(o['queue_length']),
                'c_definitions': o['body']
            })
            network['topics'].append(topic)
        elif o['type'] == 'node':
            node = OrderedDict({
                'name': o['name'],
                'publish': o['body']['publish'],
                'subscribe': o['body']['subscribe']
            })
            network['network']['nodes'].append(node)
        elif o['type'] == 'partition':
            partition = OrderedDict({
                'name': o['name'],
                'id': int(o['partition_id']),
                'nodes': o['body']
            })
            network['network']['partitions'].append(partition)
    buffer = io.StringIO()
    json.dump(network, buffer, indent=2)
    schema_path = Path(__file__).parent / 'schema' / 'topic.schema.json'
    schema = json.load(schema_path.open('r'))
    try:
        jsonschema.validate(network, schema)
        json.dump(network, output.open('w'), indent=2)
    except jsonschema.ValidationError as e:
        print(e)
        print(buffer.getvalue())
        raise


def main(file: Path, out_file: Path):
    result = parser.parse(file.read_text())
    code_generator(result, out_file)


def test_topic_parser():
    segment = """
topic control(32) {
    unsigned int id;
    unsigned int len;
    unsigned int sequence;
    unsigned int timestamp;
    unsigned char data[32];
}
    """
    lexer.lineno = 1
    lexer.input(segment)
    for i, tok in enumerate(lexer, 1):
        print(f'{i:3d} {tok}')

    lexer.lineno = 1
    result = parser.parse(segment)
    print(result)


if __name__ == '__main__':
    # test_topic_parser()
    ap = argparse.ArgumentParser()
    ap.add_argument('-f', '--file', type=Path, required=True,
                        help='topic descriptive file *.trs')
    ap.add_argument('-o', '--output', type=Path, required=True,
                        help='output file *.json')
    args = ap.parse_args()
    main(args.file, args.output)
