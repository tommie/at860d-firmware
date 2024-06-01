#!/usr/bin/env python3

import argparse
import os.path
import re

INCLUDE_RE = re.compile(r'\s*include\s+"([^"]+)"\s*(?:;.*)?$')
DEFINEMOD_RE = re.compile(r'\s*#define\s+module_(\w+)\s*(?:;.*)?$')
IFNDEFMOD_RE = re.compile(r'\s*ifndef\s+module_(\w+)\s*(?:;.*)?$')
ENDIF_RE = re.compile(r'\s*endif\s*(?:;.*)?$')
ERROR_RE = re.compile(r'\s*error\s+".*?is a dependency of.*?"\s*(?:;.*)?$')

MODULE_CATEGORY = {
    'adc': 'hardware',
    'airpump': 'hardware',
    'buttonpress': 'ui',
    'buttonscan': 'hardware',
    'buzzer': 'hardware',
    'calibration': 'mode',
    'cooldown': 'mode',
    'display': 'process',
    'eeprom': 'hardware',
    'fault': 'process',
    'frontpanel': 'ui',
    'heater': 'hardware',
    'int': 'os',
    'irq': 'os',
    'knob': 'process',
    'led': 'hardware',
    'pid': 'algorithm',
    'power': 'hardware',
    'selftest': 'mode',
    'standby': 'mode',
    'stdlib': 'algorithm',
    'switch': 'hardware',
    'temp': 'process',
    'tempcontrol': 'process',
    'timer0': 'os',
    'timer1': 'os',
    'triac_pfc': 'algorithm',
    'triac_zcc': 'algorithm',
    'userconfig': 'process',
    'wdt': 'os',
    'zerocross': 'process',
}

CATEGORY_NODE_STYLES = {
    'mode': 'style=filled,fillcolor="#fff2cc"',
    'ui': 'style=filled,fillcolor="#ffe599"',
    'process': 'style=filled,fillcolor="#ffd966"',
    'hardware': 'style=filled,fillcolor="#f1c232"',
    'algorithm': 'style=filled,fillcolor="#f1c232"',
    'os': 'style=filled,fillcolor="#d9d2e9"',
}

def main():
    argp = argparse.ArgumentParser()
    argp.add_argument('modules', type=argparse.FileType('r'), help='Path to the modules.inc file')
    argp.add_argument('--color', choices=['light', 'dark'], default='dark', help='Color scheme')
    args = argp.parse_args()

    dirname = os.path.dirname(args.modules.name)

    print('digraph Modules {')
    print('\tnodesep=0.15;')
    print('\tranksep=0;')
    print('\tratio=0.5;')
    print('\tcolor="white";')
    print('\tbgcolor="transparent";')
    print('\tnode [fontsize=28,shape=box,color="#777777"];')
    if args.color == 'dark':
        print('\tedge [color="white",style=bold];')

    for line in args.modules:
        line = line.rstrip()
        m = INCLUDE_RE.match(line)
        if not m: continue

        with open(os.path.join(dirname, m.group(1))) as modf:
            mod = None
            cat = None
            dep = None
            for line in modf:
                line = line.rstrip()
                m = DEFINEMOD_RE.match(line)
                if m:
                    mod = m.group(1)
                    cat = MODULE_CATEGORY[mod]
                    continue
                m = IFNDEFMOD_RE.match(line)
                if m:
                    dep = m.group(1)
                    continue
                m = ENDIF_RE.match(line)
                if m:
                    dep = None
                    continue
                if not dep: continue
                m = ERROR_RE.match(line)
                if m:
                    assert mod, f'No #define module_* found in {modf.name}'
                    print(f'\t"{mod}" -> "{dep}" [samehead="{cat}",sametail="{MODULE_CATEGORY[dep]}"];')

            if mod:
                # Also checks that MODULE_CATEGORY contains all modules.
                print(f'\t"{mod}" [label="{mod}",{CATEGORY_NODE_STYLES.get(cat, "")},group="{cat}"];')

    print('}')



if __name__ == '__main__':
    main()
