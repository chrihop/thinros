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
    'FLATTEN', 'NODE', 'NETWORK',
    'IDENTIFIER', 'NUMBER', 'STRING', 'COMMENT',
    'LBRACKET', 'RBRACKET', 'LBRACE', 'RBRACE', 'COMMA', 'COLON',
    'SEMICOLON', 'INTO'
)

t_ignore = ' \t\r'


def t_COMMENT(t):
    r'\#.*'
    pass


def t_FLATTEN(t):
    r'flatten'
    return t


def t_NODE(t):
    r'node'
    return t


def t_NETWORK(t):
    r'network'
    return t


def t_IDENTIFIER(t):
    r'[a-zA-Z_][a-zA-Z0-9_]*'
    return t


def t_NUMBER(t):
    r'\d+'
    t.value = int(t.value)
    return t


def t_STRING(t):
    r'\"([^\\\n]|(\\.))*?\"'
    t.value = t.value[1:-1]
    return t


def t_LBRACKET(t):
    r'\['
    return t


def t_RBRACKET(t):
    r'\]'
    return t


def t_LBRACE(t):
    r'\{'
    return t


def t_RBRACE(t):
    r'\}'
    return t


def t_COMMA(t):
    r','
    return t


def t_COLON(t):
    r':'
    return t


def t_SEMICOLON(t):
    r';'
    return t


def t_INTO(t):
    r'>'
    return t


def t_newline(t):
    r'\n+'
    t.lexer.lineno += len(t.value)


def t_error(t):
    print(f"Illegal character '{t.value[0]}'")
    t.lexer.skip(1)


lexer = lex.lex()


def p_program(p):
    '''
    program : block program
            | block
    '''
    if len(p) == 3:
        p[0] = [p[1]] + p[2]
    else:
        p[0] = [p[1]]


def p_block(p):
    '''
    block : flatten_rule
          | network_statement
    '''
    p[0] = p[1]


def p_network_statement(p):
    '''
    network_statement : NETWORK STRING SEMICOLON
    '''
    p[0] = {
        'network': p[2],
        'lineno': p.lineno(1),
    }


def p_flatten_rule(p):
    '''
    flatten_rule : FLATTEN LBRACE flatten_rule_body RBRACE INTO NODE IDENTIFIER SEMICOLON
    '''
    p[0] = {
        'flatten': p[3],
        'into': p[7],
        'lineno': p.lineno(1),
    }


def p_flatten_rule_body(p):
    '''
    flatten_rule_body : node flatten_rule_body
                      | node
    '''
    if len(p) == 3:
        p[0] = [p[1]] + p[2]
    else:
        p[0] = [p[1]]


def p_node(p):
    '''
    node : NODE IDENTIFIER SEMICOLON
    '''
    p[0] = p[2]


def p_error(p):
    if p:
        print(f"Syntax error at '{p.value}' type={p.type} lineno={p.lineno}"
              f" lexpos={p.lexpos}")
        raise SyntaxError(f'Syntax error {p.value}')
    else:
        print('Syntax error at EOF')


flatten_parser = yacc.yacc()


def test_parser():
    test = '''
    network "test.json";
    
    flatten {
        node pub_a;
        node pipe_a;
        node sub_c;
    } > node sub_c;
    
    flatten {
        node pub_b;
        node pipe_b;
        node sub_c;
    } > node sub_c;
    '''
    result = flatten_parser.parse(test)
    pprint(result)


flatten_spec = ''


def code_generator(ast) -> str:
    global flatten_spec
    flatten = OrderedDict({
        '$schema': 'https://json-schema.org/draft/2020-12/schema',
        'flt': flatten_spec,
        'networks': [],
        'rules': [],
    })
    for o in ast:
        if 'network' in o:
            flatten['networks'].append({'file': o['network'], 'lineno': o['lineno']})
        elif 'flatten' in o:
            flatten['rules'].append({'flatten': o['flatten'], 'into': o['into'], 'lineno': o['lineno']})
    return json.dumps(flatten, indent=2)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-f', '--file', type=Path,
                        help='flattening spec file *.flt', required=True)
    parser.add_argument('-d', '--dir', type=Path,
                        help='directory containing network description files')
    parser.add_argument('-o', '--output', type=Path,
                        help='output network description file of flatten network',
                        required=True)
    args = parser.parse_args()

    global flatten_spec
    flatten_spec = args.file.absolute().as_posix()

    search_path = [args.file.parent]
    if args.dir.exists():
        search_path.append(args.dir)

    with open(args.file, 'r') as f:
        source = f.read()
    ast = flatten_parser.parse(source)
    flatten = code_generator(ast)
    with open(args.output, 'w') as f:
        f.write(flatten)


if __name__ == '__main__':
    main()

