# lingua: British English tongue twisters and jokes

`lingua` is a small terminal program, in the spirit of the classic Unix
`fortune`. You type `lingua` in the terminal and it shows a tongue twister
to practise. With the `-f` flag you get a funny joke about speech instead.
It is a fun way to improve spoken **British English**.

The owner is learning C, so this project should be a good, readable
example of simple C.

## Language: C only

- The program must be written **only in C** (C99 or newer).
- Use only the C standard library. POSIX functions are fine.
- No other languages in the program: no Python, shell scripts, or code
  generators. The only extra files allowed are `Makefile`, this file, a
  `README.md` and the data files `twisters.txt` and `jokes.txt`.
- No external libraries or dependencies.
- Code must compile with **no warnings** under
  `gcc -Wall -Wextra -std=c99`.

## British English focus

- All text the program prints, and all text in the data files, uses
  **British spelling and vocabulary**: colour, favourite, practise (verb),
  practice (noun), whilst, mum, biscuit, queue, lorry, flat, and so on.
- Twisters should be chosen for British pronunciation (for example
  "Red lorry, yellow lorry", and sounds like the British "r", "th" and
  "w") and may use British places, food and culture.
- Jokes should have a British flavour: gentle, witty, a bit dry, never
  rude or offensive.
- Messages use a friendly British tone: "Brilliant!", "Smashing!",
  "Not quite, have another go!", "Cheerio!".

## Files

| File           | Purpose                                            |
| -------------- | -------------------------------------------------- |
| `lingua.c`     | The whole program                                  |
| `twisters.txt` | All the tongue twisters (data)                     |
| `jokes.txt`    | All the funny jokes about speech and language (data) |
| `Makefile`     | Build with `make`, install with `make install`     |
| `AGENTS.md`    | These instructions                                 |

If a file called `twister.c` already exists, use it as a starting point,
then rename it to `lingua.c`. It contains a "twister of the day" feature
which must be **removed** (see Behaviour).

## Easy to update (most important rule)

The program must be easy to update **just by adding new tongue twisters
or jokes**. Adding one must never require touching or recompiling the
C code.

- All twisters live in `twisters.txt` and all jokes in `jokes.txt`.
  **Never hardcode twisters or jokes in the `.c` file.**
- Format for both files: plain text, one entry per block, entries
  separated by a line containing only `%` (the same format as `fortune`).
  An entry may span several lines, which is useful for question and
  answer jokes.

  `twisters.txt`:
  ```
  She sells seashells by the seashore.
  %
  Peter Piper picked a peck of pickled peppers.
  %
  Red lorry, yellow lorry.
  ```

  `jokes.txt`:
  ```
  Why did the actor say "break a leg" to the tongue twister champion?
  Because he wanted a clean get-up-and-say.
  %
  I told my mum I was going to practise my tongue twisters.
  She said, "Don't get tongue-tied about it."
  ```

- The program reads the file at runtime, counts the entries, and works
  with any number of them. It must not have a fixed maximum, so use
  dynamic memory (`malloc`/`realloc`) and free it before exiting.
- Blank lines and extra spaces around the `%` separators must not break
  anything.
- **Whenever you update the program, also add some new twisters and some
  new jokes** to the data files. Keep twisters a mix of easy, medium and
  hard. Keep everything fun, friendly and British. Do not add an entry
  that is already in the file.

## Where the program looks for the data files

For each data file, look in this order and use the first that exists:

1. The path given on the command line (`--twisters FILE` or `--jokes FILE`)
2. `./twisters.txt` or `./jokes.txt` (the current folder)
3. `~/.local/share/lingua/`
4. `/usr/local/share/lingua/`

If none is found, print a clear, friendly error saying where it looked,
then exit with a non-zero code.

## Usage

```
lingua                 a random tongue twister (new every time)
lingua -p              a random twister, then practise it
lingua -f              a funny joke about speech or language (no twister)
lingua -h              help
lingua --twisters FILE use a different twisters file
lingua --jokes FILE    use a different jokes file
```

## Behaviour

- **Default (`lingua` with no options):** show one **random** tongue
  twister. It is a new one every time you run it. There is **no
  "twister of the day"**: remove all date-based logic and the old `-r`
  option.
- **Truly different each run:**
  - Seed the random generator with something that differs between runs
    (for example the time combined with the process ID), so two runs in
    the same second still differ.
  - Do not show the same entry twice in a row when there are two or more
    entries. Remember the last one in a small state file, for example
    `~/.cache/lingua/last`. If that file cannot be read or written,
    carry on silently without it.
- **Joke mode (`-f`):** show one random joke from `jokes.txt` and
  **do not** show a twister. Apply the same "different each time" rules
  as above. `-f` and `-p` together are not allowed: print a friendly
  message explaining why.
- **Practice mode (`-p`):**
  - Show the random twister, then let the user type it back and press
    Enter.
  - Compare it with the original **ignoring capital letters,
    punctuation and extra spaces**, so only the words matter.
  - If it is correct, say so (for example "Brilliant!") and show how many
    seconds it took, then let them try again, faster.
  - If it is wrong, show a gentle, funny message and let them retry.
    Never be harsh.
  - An empty line ends practice with a friendly goodbye ("Cheerio!").
- The output should be short, clean and a little fun.

## Code style

- Keep the code **simple and well commented**. Explain the why, not just
  the what. The owner is learning C.
- Small functions with clear names. Avoid clever tricks.
- Use one function to load either data file, so twisters and jokes share
  the same code.
- Check every return value that can fail (`fopen`, `malloc`, `fgets`).
- Never read input into a fixed buffer without a size limit. No `gets`,
  no unbounded `scanf("%s")`.
- No memory leaks or undefined behaviour. Check with
  `valgrind` or `-fsanitize=address,undefined` when possible.

## Build, run and test

```
make            # builds ./lingua
./lingua        # a random twister
./lingua -f     # a funny joke
./lingua -p     # practise a twister
make install    # optional: installs to ~/.local/bin and ~/.local/share/lingua
```

Before finishing any change:

1. It compiles with no warnings.
2. `./lingua`, `./lingua -f`, `./lingua -p` and `./lingua -h` all work.
3. Running `./lingua` several times gives different twisters, and never
   the same one twice in a row.
4. It works with data files that have 1 entry, many entries, and a
   missing file (clear error, no crash).
5. All text and new entries use British spelling.
6. New twisters and jokes have been added to the data files.

## Do not

- Do not use any language other than C for the program.
- Do not hardcode twisters or jokes in the source.
- Do not bring back the "twister of the day" or the `-r` option.
- Do not use American spelling or slang.
- Do not add dependencies or a complicated build system.
- Do not make the program require internet access.
