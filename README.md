# lingua

A small terminal program in the spirit of `fortune` that shows you a random
British English tongue twister, or a joke about speech. Practise saying it to
improve your spoken English!

## Building

```bash
make
```

This will create the `lingua` executable.

## Usage

```bash
lingua                 # A random tongue twister, new every time
lingua -p              # Practise a twister: type it back, as fast as you can
lingua -f              # A funny joke about speech or language
lingua -h              # Show help
lingua --twisters FILE # Use a different twisters file
lingua --jokes FILE    # Use a different jokes file
```

It never shows the same entry twice in a row. The last one is remembered in
`~/.cache/lingua/`, so this works between runs.

## Where the data files are looked for

For each data file the first one found is used:

1. the path given with `--twisters` or `--jokes`
2. `./twisters.txt` or `./jokes.txt`
3. `~/.local/share/lingua/`
4. `/usr/local/share/lingua/`

## Installation

```bash
make install
```

Installs to `~/.local/bin/lingua` and copies `twisters.txt` and `jokes.txt`
to `~/.local/share/lingua/`. Use `make uninstall` to remove them again.

## Adding more twisters and jokes

Just edit `twisters.txt` or `jokes.txt` and add new entries separated by `%`
lines. No recompilation needed!

Format:
```
First twister here
%
Second twister here,
it can span multiple lines
%
Third twister
```