#!/usr/bin/env python3
"""Convert switch statements to if-else chains in C source files."""

import re
import sys

def find_matching_brace(text, start):
    """Find the position after the matching closing brace."""
    depth = 1
    pos = start + 1
    while pos < len(text) and depth > 0:
        ch = text[pos]
        if ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
        elif ch in ('"', "'"):
            quote = ch
            pos += 1
            while pos < len(text) and text[pos] != quote:
                if text[pos] == '\\':
                    pos += 1
                pos += 1
        elif ch == '/' and pos + 1 < len(text):
            if text[pos + 1] == '/':
                # Line comment
                while pos < len(text) and text[pos] != '\n':
                    pos += 1
            elif text[pos + 1] == '*':
                # Block comment
                pos += 2
                while pos + 1 < len(text) and not (text[pos] == '*' and text[pos + 1] == '/'):
                    pos += 1
                pos += 1
        pos += 1
    return pos if depth == 0 else -1

def parse_switch_body(body):
    """Parse a switch body into a list of (conditions, statements) pairs."""
    # Tokenize into case groups
    groups = []
    current_conds = []
    current_stmts = []
    is_default = False
    
    i = 0
    while i < len(body):
        # Skip whitespace
        while i < len(body) and body[i] in ' \t\n\r':
            i += 1
        if i >= len(body):
            break
        
        # Check for case
        m = re.match(r'case\s+', body[i:])
        if m:
            # Find the colon
            rest = body[i + m.end():]
            # Handle complex case expressions (e.g., case TOKEN_A: case TOKEN_B:)
            colon_pos = rest.find(':')
            if colon_pos >= 0:
                cond = rest[:colon_pos].strip()
                current_conds.append(cond)
                i = i + m.end() + colon_pos + 1
                continue
            else:
                i += 1
                continue
        
        # Check for default
        if body[i:i+7] == 'default':
            rest = body[i+7:].lstrip()
            if rest and rest[0] == ':':
                is_default = True
                i = i + 7 + (len(body[i+7:]) - len(rest)) + 1
                continue
        
        # It's a statement - collect until next case/default/end
        # Find the end of this statement block
        stmt_start = i
        depth = 0
        while i < len(body):
            ch = body[i]
            if ch == '{':
                depth += 1
            elif ch == '}':
                if depth > 0:
                    depth -= 1
                else:
                    break
            elif ch in ('"', "'"):
                quote = ch
                i += 1
                while i < len(body) and body[i] != quote:
                    if body[i] == '\\':
                        i += 1
                    i += 1
            elif depth == 0:
                # Check if we hit a case or default at depth 0
                m2 = re.match(r'\s*case\s+', body[i:])
                m3 = re.match(r'\s*default\s*:', body[i:])
                if m2 or m3:
                    break
            i += 1
        
        stmt = body[stmt_start:i].strip()
        if stmt:
            if is_default or current_conds:
                current_stmts.append(stmt)
        
        # Check if next is case/default
        saved_i = i
        while i < len(body) and body[i] in ' \t\n\r':
            i += 1
        
        m2 = re.match(r'case\s+', body[i:])
        m3 = re.match(r'default\s*:', body[i:])
        
        if m2 or m3 or i >= len(body):
            # End of this group
            if current_conds or is_default:
                groups.append((list(current_conds), list(current_stmts), is_default))
                current_conds = []
                current_stmts = []
                is_default = False
            i = saved_i  # Go back to re-process
        else:
            i = saved_i
    
    # Flush remaining
    if current_conds or is_default:
        groups.append((list(current_conds), list(current_stmts), is_default))
    
    return groups

def convert_switch(text):
    """Convert all switch statements in text to if-else chains."""
    result = text
    
    while True:
        # Find next switch
        m = re.search(r'(\n[ \t]*)switch\s*\(([^)]+)\)\s*\{', result)
        if not m:
            break
        
        indent = m.group(1)
        inner_indent = indent + "    "
        expr = m.group(2)
        
        # Find the opening brace
        brace_pos = result.index('{', m.start())
        
        # Find matching closing brace
        end_pos = find_matching_brace(result, brace_pos)
        if end_pos < 0:
            print(f"Warning: unmatched brace in switch", file=sys.stderr)
            break
        
        body = result[brace_pos + 1:end_pos - 1]
        
        # Parse the body
        groups = parse_switch_body(body)
        
        # Generate if-else chain
        output_lines = []
        first = True
        default_stmts = None
        
        for conds, stmts, is_default in groups:
            if is_default:
                default_stmts = stmts
                continue
            
            if not conds:
                continue
            
            # Build condition: expr == cond1 || expr == cond2 || ...
            cond_parts = [f"{expr} == {c}" for c in conds]
            if len(cond_parts) == 1:
                cond_str = cond_parts[0]
            else:
                cond_str = " || ".join(cond_parts)
            
            if first:
                output_lines.append(f"{indent}if ({cond_str}) {{")
                first = False
            else:
                output_lines.append(f"{indent}}} else if ({cond_str}) {{")
            
            for stmt in stmts:
                # Re-indent statements
                for line in stmt.split('\n'):
                    stripped = line.strip()
                    if stripped:
                        output_lines.append(f"{inner_indent}{stripped}")
        
        if default_stmts:
            output_lines.append(f"{indent}}} else {{")
            for stmt in default_stmts:
                for line in stmt.split('\n'):
                    stripped = line.strip()
                    if stripped:
                        output_lines.append(f"{inner_indent}{stripped}")
        
        if output_lines:
            output_lines.append(f"{indent}}}")
        
        replacement = '\n'.join(output_lines)
        result = result[:m.start()] + replacement + result[end_pos:]
    
    # Remove break statements from converted switches
    # Standalone break; on its own line
    result = re.sub(r'^\s*break;\s*$', '', result, flags=re.MULTILINE)
    # break; on same line as other code (after a ;)
    result = re.sub(r';\s*break;', ';', result)
    # Remove excessive blank lines
    result = re.sub(r'\n{3,}', '\n\n', result)
    
    return result

def process_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()
    
    original = content
    content = convert_switch(content)
    
    if content != original:
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"Converted: {filepath}")
        return True
    else:
        print(f"No changes: {filepath}")
        return False

if __name__ == '__main__':
    if len(sys.argv) > 1:
        for f in sys.argv[1:]:
            process_file(f)
    else:
        files = [
            'src/parser/ast.c',
            'src/ir/lowerer.c',
            'src/backend/codegen.c',
            'src/sema/analyzer.c',
            'src/ir/ir.c',
            'src/backend/wasm_emit.c',
            'src/backend/wasm_codegen.c',
        ]
        for f in files:
            process_file(f)
