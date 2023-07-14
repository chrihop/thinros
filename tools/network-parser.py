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
    'STARTUP', 'SHUTDOWN', 'ON_SPIN', 'DEFAULT',
    'IDENTIFIER', 'STATEMENT', 'NUMBER', 'STRING', 'COMMENT',
    'LPAREN', 'RPAREN', 'LBRACE', 'RBRACE', 'LBRACKET', 'RBRACKET',
    'COMMA', 'EQUALS', 'MAPS_TO', 'SEMICOLON', 'DOUBLE_COLON', 'COLON'
)

t_ignore = ' \t\r'


def t_COMMENT(t):
    r'\#.*'
    pass


def t_TOPIC(t):
    r'\btopic\b'
    return t


def t_NODE(t):
    r'\bnode\b'
    return t


def t_PARTITION(t):
    r'\bpartition\b'
    return t


def t_MISC(t):
    r'\bmisc\b'
    return t


def t_PUBLISH(t):
    r'\bpublish\b'
    return t


def t_SUBSCRIBE(t):
    r'\bsubscribe\b'
    return t


def t_STARTUP(t):
    r'\bstartup\b'
    return t


def t_SHUTDOWN(t):
    r'\bshutdown\b'
    return t


def t_ON_SPIN(t):
    r'\bon_spin\b'
    return t


def t_DEFAULT(t):
    r'\bdefault\b'
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


def t_MAPS_TO(t):
    r'->'
    return t


def t_SEMICOLON(t):
    r';'
    return t


def t_DOUBLE_COLON(t):
    r'::'
    return t


def t_COLON(t):
    r':'
    return t


def t_newline(t):
    r'\n+'
    t.lexer.lineno += len(t.value)


def t_error(t):
    print(f"Illegal character `{t.value[0]}`")
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
    node_block : NODE IDENTIFIER LBRACE node_body RBRACE
               | NODE IDENTIFIER LBRACE RBRACE
    """
    if len(p) == 6:
        p[0] = {
            'type': 'node',
            'name': p[2],
            'body': p[4]
        }
    else:
        p[0] = {
            'type': 'node',
            'name': p[2],
            'body': []
        }


def p_node_body(p):
    """
    node_body : node_body_block node_body
              | node_body_block
    """
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0] = [p[1]] + p[2]


def p_node_body_block(p):
    """
    node_body_block : publish_block
                    | subscribe_block
                    | on_startup_block
                    | on_shutdown_block
                    | on_spin_block
    """
    p[0] = p[1]


def p_publish_block(p):
    """
    publish_block : PUBLISH EQUALS LBRACE advertise_body RBRACE
    """
    p[0] = {
        'type': 'publish-list',
        'body': p[4]
    }


def p_advertise_body(p):
    """
    advertise_body : advertise_body COMMA advertise_item
                   | advertise_body COMMA
                   | advertise_item
    """
    if len(p) == 2:
        p[0] = [p[1]]
    elif len(p) == 3:
        p[0] = p[1]
    else:
        p[0] = p[1] + [p[3]]


def p_advertise_item(p):
    """
    advertise_item : IDENTIFIER
    """
    p[0] = p[1]


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
    action_body : action_body COMMA action_item
                | action_body COMMA
                | action_item
    """
    if len(p) == 2:
        p[0] = [p[1]]
    elif len(p) == 3:
        p[0] = p[1]
    else:
        p[0] = p[1] + [p[3]]


def p_action_item(p):
    """
    action_item : IDENTIFIER MAPS_TO method LPAREN RPAREN
                | IDENTIFIER MAPS_TO default_method
    """
    p[0] = {
        'topic': p[1],
        'callback': p[3]
    }


def p_method(p):
    """
    method : IDENTIFIER DOUBLE_COLON IDENTIFIER
           | DOUBLE_COLON IDENTIFIER
           | IDENTIFIER
    """
    p[0] = {
        'name': p[len(p) - 1],
        'namespace': p[1] if len(p) == 4 else '',
    }


def p_default_method(p):
    """
    default_method : IDENTIFIER DOUBLE_COLON DEFAULT
                   | DOUBLE_COLON DEFAULT
                   | DEFAULT
    """
    p[0] = {
        'name': 'default',
        'namespace': p[1] if len(p) == 4 else '',
    }


def p_on_startup_block(p):
    """
    on_startup_block : STARTUP EQUALS LBRACE callback_body RBRACE
                     | STARTUP EQUALS default_method SEMICOLON
    """
    p[0] = {
        'type': 'on-startup',
        'body': p[4] if len(p) == 6 else [p[3]]
    }


def p_callback_body(p):
    """
    callback_body : callback_body COMMA callback_item
                  | callback_body COMMA
                  | callback_item
    """
    if len(p) == 2:
        p[0] = [p[1]]
    elif len(p) == 3:
        p[0] = p[1]
    else:
        p[0] = p[1] + [p[3]]


def p_callback_item(p):
    """
    callback_item : method LPAREN RPAREN
    """
    p[0] = p[1]


def p_on_shutdown_block(p):
    """
    on_shutdown_block : SHUTDOWN EQUALS LBRACE callback_body RBRACE
                        | SHUTDOWN EQUALS default_method SEMICOLON
    """
    p[0] = {
        'type': 'on-shutdown',
        'body': p[4] if len(p) == 6 else [p[3]]
    }


def p_on_spin_block(p):
    """
    on_spin_block : ON_SPIN EQUALS LBRACE callback_body RBRACE
                  | ON_SPIN EQUALS default_method SEMICOLON
    """
    p[0] = {
        'type': 'on-spin',
        'body': p[4] if len(p) == 6 else [p[3]]
    }


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


class ParserError(Exception):
    def __init__(self, message, token=None, lineno=-1, lexpos=-1):
        self.message = message
        self.token = token
        self.lineno = lineno
        self.lexpos = lexpos


def p_error(p):
    if p:
        raise ParserError(f'Syntax error at `{p.value}` type: {p.type} line: {p.lineno}'
              f' {p.lexpos}', p, p.lineno, p.lexpos)
    else:
        raise ParserError('Syntax error at EOF')


parser = yacc.yacc()

network_description_file = ""


def code_generator(ast, output: Path):
    global network_description_file

    network = OrderedDict()
    network['$schema'] = "https://github.com/chrihop/thinros/blob/v2/lib/topic.schema.json",
    network['trs'] = network_description_file
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
            on_startup = []
            on_shutdown = []
            on_spin = []
            pub_topics = []
            sub_topics = []
            for b in o['body']:
                if b['type'] == 'on-startup':
                    on_startup.extend(b['body'])
                elif b['type'] == 'on-shutdown':
                    on_shutdown.extend(b['body'])
                elif b['type'] == 'on-spin':
                    on_spin.extend(b['body'])
                elif b['type'] == 'publish-list':
                    pub_topics = b['body']
                elif b['type'] == 'subscribe-list':
                    sub_topics = b['body']
            node = OrderedDict({
                'name': o['name'],
                'publish': pub_topics,
                'subscribe': sub_topics,
            })
            if on_startup:
                node['on_startup'] = on_startup
            if on_shutdown:
                node['on_shutdown'] = on_shutdown
            if on_spin:
                node['on_spin'] = on_spin

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
    global network_description_file
    network_description_file = file.absolute().as_posix()

    try:
        result = parser.parse(file.read_text())
    except ParserError as e:
        print(e.message)
        if e.lineno != -1:
            texts = file.read_text()
            lines = texts.splitlines()
            line_from = max(e.lineno - 5, 0)
            line_to = min(e.lineno + 5, len(lines))
            for i in range(line_from, line_to):
                if i == e.lineno - 1:
                    print(f'-> {i+1:3d} {lines[i]}')
                    last_cr = texts.rfind('\n', 0, e.lexpos)
                    if last_cr < 0:
                        last_cr = 0
                    column = (e.lexpos - last_cr) + 6
                    print(' ' * column + '^')
                    print(' ' * column + '+', end='')
                    print('-' * 5, end='')
                    print(f' *{e.token.type}({e.token.value}), ', end='')
                    next_tokens = list(lexer.token() for _ in range(5))
                    next_tokens = next_tokens[:min(5, len(next_tokens))]
                    next_tokens = [f'{t.type}({t.value})' for t in next_tokens if t is not None]
                    print(f'{", ".join(next_tokens)}')
                else:
                    print(f'   {i+1:3d} {lines[i]}')
        return
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
