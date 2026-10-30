
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
 
## Support the project
 
lingua is free and always will be. If it helps you practise your English and
you would like to say thanks, you are welcome to donate with crypto.
Every bit is appreciated, and so is a star on the repository!
 
| Coin           | Network  | Address                                        |
| -------------- | -------- | -----------------------------------------------|
| Bitcoin  (BTC) | Bitcoin  | `bc1q9je7dud4ygfysjyf0jnmapx7fl7drhrx7nxc2a`   |
| Litecoin (LTC) | Litecoin | `ltc1qmawclx33uza25vnpsv0lzl6tk0qhswluwhzkf7`  |
 
Please double-check the network before sending. Coins sent over the wrong
network cannot be recovered.
