#!/usr/bin/env python3
"""Build separate focused tests without changing the assignment CMake targets."""
import argparse
import json
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--only', nargs='+', help='Function names without bm_ or test_')
    parser.add_argument('--build-dir', default='build/focused')
    parser.add_argument('--compiler', default='clang')
    parser.add_argument('--sanitize', choices=['address', 'undefined', 'address,undefined'])
    parser.add_argument('--timeout', type=float, default=30)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    output = Path(args.build_dir)
    if not output.is_absolute():
        output = root / output
    output.mkdir(parents=True, exist_ok=True)
    sources = sorted((root / 'tests').glob('test_*.c'))
    sources = [p for p in sources if p.stem not in {'test_game', 'test_rules_public'}]
    if args.only:
        wanted = {'test_' + name.removeprefix('bm_').removeprefix('test_') for name in args.only}
        missing = wanted - {p.stem for p in sources}
        if missing:
            parser.error('Unknown focused tests: ' + ', '.join(sorted(missing)))
        sources = [p for p in sources if p.stem in wanted]
    results = []
    for source in sources:
        binary = output / source.stem
        command = [args.compiler, '-std=c99', '-Wall', '-Wextra', '-g',
                   '-I', str(root / 'src'), str(source), str(root / 'src/rules.c'),
                   str(root / 'src/game.c'), '-lm', '-o', str(binary)]
        if args.sanitize:
            command += ['-fsanitize=' + args.sanitize, '-fno-sanitize-recover=all',
                        '-fno-omit-frame-pointer']
        record = {'name': source.stem, 'compile_command': command}
        try:
            compiled = subprocess.run(command, capture_output=True, text=True, timeout=args.timeout)
            (output / (source.stem + '-build.log')).write_text(compiled.stdout + compiled.stderr)
            record['compile_exit'] = compiled.returncode
            if compiled.returncode:
                record['status'] = 'compile_failed'
                print(compiled.stderr, file=sys.stderr)
            else:
                tested = subprocess.run([str(binary)], capture_output=True, text=True, timeout=args.timeout)
                (output / (source.stem + '.log')).write_text(tested.stdout + tested.stderr)
                record.update(status='passed' if tested.returncode == 0 else 'failed',
                              test_exit=tested.returncode, summary=tested.stdout.strip())
                print(tested.stdout.strip(), flush=True)
                if tested.stderr:
                    print(tested.stderr, file=sys.stderr)
        except subprocess.TimeoutExpired:
            record['status'] = 'timeout'
            print(source.stem + ': timed out', flush=True)
        results.append(record)
        (output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
        if record['status'] != 'passed':
            return 1
    print(f'Focused executables: {len(results)} passed, 0 failed', flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
