/*
 * lingua - British English tongue twisters and jokes.
 *
 * The program keeps no twisters or jokes of its own: it reads them at run
 * time from twisters.txt and jokes.txt, so new entries never need a rebuild.
 * Both files use the same simple format as fortune: one entry per block and
 * a line with just "%" between the blocks.
 */

#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* One entry from a data file: the whole text of a twister or a joke. */
typedef struct {
    char *text;
} Entry;

/* All the entries of one data file, held in a growable array. */
typedef struct {
    Entry *entries;
    size_t count;
    size_t capacity;
} EntryList;

/*
 * Growing text buffers
 * -------------------
 * Reading a file line by line means the text of an entry can be any length,
 * so we never use a fixed size array. We start with this much room and double
 * it whenever it is not enough.
 */
#define START_CAPACITY 256

/* A growable piece of text. */
typedef struct {
    char *data;   /* always NULL, or a string ending in '\0' */
    size_t len;   /* characters used, not counting the '\0' */
    size_t cap;   /* characters that fit, including room for the '\0' */
} Text;

/* Start an empty buffer. Nothing is allocated until it is needed. */
static void text_start(Text *text) {
    text->data = NULL;
    text->len = 0;
    text->cap = 0;
}

/* Make sure there is room for one more character. Returns 0, or -1 if the
 * computer has run out of memory. */
static int text_reserve(Text *text, size_t extra) {
    size_t needed = text->len + extra + 1; /* one byte for the '\0' */
    char *bigger;
    size_t new_cap;

    if (needed <= text->cap) {
        return 0;
    }

    /* Start small, then keep doubling: that way a very long entry costs only
     * as many reallocations as it needs. */
    new_cap = text->cap == 0 ? START_CAPACITY : text->cap;
    while (new_cap < needed) {
        new_cap *= 2;
    }

    bigger = realloc(text->data, new_cap);
    if (bigger == NULL) {
        return -1;
    }
    text->data = bigger;
    text->cap = new_cap;

    /* Keep the promise that data is always a proper string, even straight
     * after growing: an empty line must still end in '\0'. */
    text->data[text->len] = '\0';
    return 0;
}

/* Add some text to the end of a buffer. */
static int text_append(Text *text, const char *more, size_t more_len) {
    if (text_reserve(text, more_len) != 0) {
        return -1;
    }
    memcpy(text->data + text->len, more, more_len);
    text->len += more_len;
    text->data[text->len] = '\0';
    return 0;
}

/* Add a line to a buffer, on its own line and without the newline. */
static int text_append_line(Text *text, const char *line) {
    if (text->len > 0 && text_append(text, "\n", 1) != 0) {
        return -1;
    }
    return text_append(text, line, strlen(line));
}

/* Throw the text away but keep the memory, ready for the next entry. */
static void text_clear(Text *text) {
    text->len = 0;
    if (text->data != NULL) {
        text->data[0] = '\0';
    }
}

/* Give the memory back. Always use this before the program ends. */
static void text_free(Text *text) {
    free(text->data);
    text_start(text);
}

/* Paths */

/* Join a folder and a file name, such as "dir/name", as a new string.
 * A '/' is put in between, so do not add one yourself. */
static char *path_join(const char *dir, const char *name) {
    size_t dir_len = strlen(dir);
    size_t name_len = strlen(name);
    char *path = malloc(dir_len + 1 + name_len + 1);

    if (path == NULL) {
        return NULL;
    }
    memcpy(path, dir, dir_len);
    path[dir_len] = '/';
    memcpy(path + dir_len + 1, name, name_len + 1);
    return path;
}

/* Can this file be opened for reading? */
static int file_is_readable(const char *path) {
    FILE *f = fopen(path, "r");

    if (f == NULL) {
        return 0;
    }
    fclose(f);
    return 1;
}

/* Find a data file and say where we looked if we cannot. Works for both
 * twisters.txt and jokes.txt, which is why the name is passed in.
 * Returns a new string that the caller must free, or NULL. */
static char *find_data_file(const char *given, const char *name) {
    const char *home = getenv("HOME");
    char *path;

    /* A file named on the command line must exist, so do not go looking
     * elsewhere: the user asked for that one. */
    if (given != NULL && given[0] != '\0') {
        if (file_is_readable(given)) {
            return strdup(given);
        }
        fprintf(stderr, "Sorry, I cannot open '%s'.\n", given);
        return NULL;
    }

    path = path_join(".", name);
    if (path == NULL) {
        return NULL;
    }
    if (file_is_readable(path)) {
        return path;
    }
    free(path);

    if (home != NULL && home[0] != '\0') {
        char *share = path_join(home, ".local/share/lingua");

        if (share == NULL) {
            return NULL;
        }
        path = path_join(share, name);
        free(share);
        if (path == NULL) {
            return NULL;
        }
        if (file_is_readable(path)) {
            return path;
        }
        free(path);
    }

    path = path_join("/usr/local/share/lingua", name);
    if (path == NULL) {
        return NULL;
    }
    if (file_is_readable(path)) {
        return path;
    }
    free(path);

    /* Nothing worked, so list everywhere we looked. That way the user can
     * see what to do about it. */
    fprintf(stderr, "Sorry, I cannot find %s. I looked in:\n", name);
    fprintf(stderr, "  ./%s\n", name);
    fprintf(stderr, "  ~/.local/share/lingua/%s\n", name);
    fprintf(stderr, "  /usr/local/share/lingua/%s\n", name);
    fprintf(stderr, "Try: lingua --%s FILE\n", strcmp(name, "jokes.txt") == 0
                                                    ? "jokes"
                                                    : "twisters");
    return NULL;
}

/* Remembering what we showed last
 * -------------------------------
 * The program keeps the number of the last entry in a small file under
 * ~/.cache so that two runs in a row do not pick the same one. If the file
 * cannot be written we simply carry on without it.
 */

/* Where the state file lives, creating the folders if we can.
 * Returns a new string that the caller must free, or NULL. */
static char *state_file(const char *name) {
    const char *home = getenv("HOME");
    const char *cache_home;
    char *folder;
    char *dir;
    char *path;

    if (home == NULL || home[0] == '\0') {
        return NULL;
    }

    cache_home = getenv("XDG_CACHE_HOME");
    if (cache_home != NULL && cache_home[0] != '\0') {
        folder = strdup(cache_home);
    } else {
        folder = path_join(home, ".cache");
    }
    if (folder == NULL) {
        return NULL;
    }

    dir = path_join(folder, "lingua");
    if (dir == NULL) {
        free(folder);
        return NULL;
    }

    /* Both folders are usually there already. If they are not, try to make
     * them, and if that fails do not worry: the program works without the
     * state file. */
    (void)mkdir(folder, 0700);
    (void)mkdir(dir, 0700);

    path = path_join(dir, name);
    free(dir);
    free(folder);
    return path;
}

/* Write down which entry we just showed. */
static void remember_last(const char *path, long index) {
    FILE *f;

    if (path == NULL) {
        return;
    }
    f = fopen(path, "w");
    if (f == NULL) {
        return;
    }
    (void)fprintf(f, "%ld\n", index);
    fclose(f);
}

/* Which entry did we show last time? Returns -1 when we have no idea. */
static long recall_last(const char *path) {
    FILE *f;
    char line[64];
    long index;

    if (path == NULL) {
        return -1;
    }
    f = fopen(path, "r");
    if (f == NULL) {
        return -1;
    }
    if (fgets(line, sizeof(line), f) == NULL) {
        fclose(f);
        return -1;
    }
    fclose(f);

    index = strtol(line, NULL, 10);
    if (index < 0) {
        return -1;
    }
    return index;
}

/*
 * Loading a data file
 * -------------------
 * The same code loads twisters.txt and jokes.txt: read the file a line at a
 * time, and every "%" line finishes the current entry.
 */

/* Is this line a separator? A separator holds nothing but a "%", with any
 * number of spaces around it. */
static int is_separator(const char *line) {
    size_t marks = 0;
    size_t i;

    for (i = 0; line[i] != '\0'; i++) {
        if (isspace((unsigned char)line[i])) {
            continue;
        }
        if (line[i] != '%') {
            return 0;
        }
        marks++;
    }
    return marks == 1;
}

/* Remove the spaces at both ends of a piece of text. */
static void trim_spaces(char *text) {
    char *start = text;
    size_t len;

    while (*start != '\0' && isspace((unsigned char)*start)) {
        start++;
    }
    if (start != text) {
        memmove(text, start, strlen(start) + 1);
    }

    len = strlen(text);
    while (len > 0 && isspace((unsigned char)text[len - 1])) {
        len--;
        text[len] = '\0';
    }
}

/* Make room for one more entry in the array. */
static int list_grow(EntryList *list) {
    size_t new_cap = list->capacity == 0 ? 8 : list->capacity * 2;
    Entry *bigger = realloc(list->entries, new_cap * sizeof(Entry));

    if (bigger == NULL) {
        return -1;
    }
    list->entries = bigger;
    list->capacity = new_cap;
    return 0;
}

/* Move the text we have collected into the list and start again.
 * Empty blocks are skipped, so stray "%" lines do no harm. */
static int list_add(EntryList *list, Text *text) {
    char *copy;
    size_t len;

    if (text->data == NULL || text->len == 0) {
        return 0;
    }
    trim_spaces(text->data);
    if (text->data[0] == '\0') {
        text_clear(text);
        return 0;
    }

    if (list->count == list->capacity && list_grow(list) != 0) {
        return -1;
    }

    len = strlen(text->data);
    copy = malloc(len + 1);
    if (copy == NULL) {
        return -1;
    }
    memcpy(copy, text->data, len + 1);

    list->entries[list->count].text = copy;
    list->count++;

    text_clear(text);
    return 0;
}

/* Read one line from a file into a growable buffer, without the newline.
 * Returns 0 on success, -1 at the end of the file, -2 on trouble. */
static int read_line(FILE *f, Text *line) {
    int c;

    text_clear(line);
    if (text_reserve(line, 1) != 0) {
        return -2;
    }

    while ((c = fgetc(f)) != EOF) {
        if (c == '\n') {
            break;
        }
        if (text_reserve(line, 1) != 0) {
            return -2;
        }
        line->data[line->len++] = (char)c;
        line->data[line->len] = '\0';
    }

    /* Text files sometimes end lines with CR LF, so drop the CR too. */
    if (line->len > 0 && line->data[line->len - 1] == '\r') {
        line->len--;
        line->data[line->len] = '\0';
    }

    if (c == EOF) {
        if (ferror(f)) {
            return -2;
        }
        if (line->len == 0) {
            return -1; /* nothing left in the file */
        }
    }
    return 0;
}

/* Load every entry from a data file.
 * Returns 0 on success, -1 if the file cannot be read, -2 if we run out of
 * memory. */
static int load_entries(const char *path, EntryList *list) {
    FILE *f;
    Text line;
    Text block;
    int status = 0;

    /* Start empty before anything can go wrong, so that the caller can
     * always hand the list back to free_entries(). */
    list->entries = NULL;
    list->count = 0;
    list->capacity = 0;

    f = fopen(path, "r");
    if (f == NULL) {
        return -1;
    }

    text_start(&line);
    text_start(&block);

    for (;;) {
        status = read_line(f, &line);
        if (status == -1) {
            status = 0; /* the end of the file is a normal ending */
            break;
        }
        if (status != 0) {
            break;
        }
        if (is_separator(line.data)) {
            /* A "%" ends the entry we have been building. */
            status = list_add(list, &block) == 0 ? 0 : -2;
        } else {
            status = text_append_line(&block, line.data) == 0 ? 0 : -2;
        }
        if (status != 0) {
            break;
        }
    }

    /* If the file itself went wrong, for example it turned out to be a
    * folder, that is a read problem rather than a memory problem. */
    if (status == -2 && ferror(f)) {
        status = -1;
    }

    /* The last entry in a file has no "%" after it. */
    if (status == 0 && list_add(list, &block) != 0) {
        status = -2;
    }

    text_free(&line);
    text_free(&block);
    fclose(f);
    return status;
}

/* Give back all the memory a list is using. */
static void free_entries(EntryList *list) {
    size_t i;

    for (i = 0; i < list->count; i++) {
        free(list->entries[i].text);
    }
    free(list->entries);
    list->entries = NULL;
    list->count = 0;
    list->capacity = 0;
}

/*
 * Choosing an entry
 * -----------------
 */

/* Start the random generator. The clock alone is not enough, because two
 * runs in the same second would then begin with the same numbers, so the
 * process ID is mixed in as well. */
static void seed_random(void) {
    unsigned int seed = (unsigned int)time(NULL);
    int i;

    seed = seed * 1103515245u + 12345u + (unsigned int)getpid();
    srand(seed);

    /* Throw the first few numbers away. When two seeds are close together,
     * as they are when two runs follow one another, the numbers straight
     * after seeding can be poorly mixed. */
    for (i = 0; i < 8; i++) {
        (void)rand();
    }
}

/* Pick an entry at random, but never the one we showed last time.
 * There is no state file for practice mode, so it picks freely. */
static size_t pick_index(const char *state_path, size_t count,
                         int remember) {
    long last;
    size_t index;

    if (count == 0) {
        return 0;
    }

    last = remember ? recall_last(state_path) : -1;
    index = (size_t)rand() % count;

    /* With two or more entries there is always another one to pick. */
    if (count > 1 && last >= 0 && (long)index == last) {
        index = (index + 1) % count;
    }
    if (remember) {
        remember_last(state_path, (long)index);
    }
    return index;
}

/* Load a data file, choose one entry and return a copy of it.
 * Prints a friendly message and returns NULL if that is not possible. */
static char *pick_one(const char *given, const char *name, const char *label,
                      const char *state_name, int remember) {
    EntryList list;
    char *path = find_data_file(given, name);
    char *state_path = NULL;
    size_t index;
    char *text;
    int status;

    if (path == NULL) {
        return NULL;
    }
    if (remember) {
        state_path = state_file(state_name);
    }

    status = load_entries(path, &list);
    if (status != 0) {
        if (status == -1) {
            fprintf(stderr, "Sorry, I could not read %s.\n", path);
        } else {
            fprintf(stderr, "Sorry, I ran out of memory reading %s.\n", path);
        }
        free_entries(&list); /* tidy up any entries read before the trouble */
        free(state_path);
        free(path);
        return NULL;
    }

    if (list.count == 0) {
        fprintf(stderr, "There are no %ss in %s.\n", label, path);
        free(state_path);
        free(path);
        free_entries(&list);
        return NULL;
    }

    /* Pick an entry, and remember the choice for next time. */
    index = pick_index(state_path, list.count, remember);
    text = strdup(list.entries[index].text);

    free(state_path);
    free(path);
    free_entries(&list);
    return text;
}

/*
 * Practice mode
 * -------------
 */

/* Read a line of typing from the keyboard. The buffer grows as needed, so a
 * very long answer is never cut short or run over.
 * Returns a new string that the caller must free, or NULL if the memory ran
 * out or the keyboard gave us nothing at all. */
static char *read_answer(void) {
    Text answer;
    int c;

    text_start(&answer);
    if (text_reserve(&answer, 1) != 0) {
        return NULL;
    }

    while ((c = getchar()) != EOF && c != '\n') {
        if (text_reserve(&answer, 1) != 0) {
            text_free(&answer);
            return NULL;
        }
        answer.data[answer.len++] = (char)c;
        answer.data[answer.len] = '\0';
    }
    answer.data[answer.len] = '\0';

    /* ferror() means something went wrong reading the keyboard. */
    if (ferror(stdin)) {
        text_free(&answer);
        return NULL;
    }
    if (c == EOF && answer.len == 0) {
        text_free(&answer);
        return NULL;
    }
    return answer.data;
}

/* Are these two pieces of text the same when only the words count?
 * Capitals, punctuation and any extra spaces are ignored. */
static int same_words(const char *a, const char *b) {
    size_t i = 0;
    size_t j = 0;

    for (;;) {
        /* Move along to the next word in each text. */
        while (a[i] != '\0' && !isalnum((unsigned char)a[i])) {
            i++;
        }
        while (b[j] != '\0' && !isalnum((unsigned char)b[j])) {
            j++;
        }

        /* Both texts must run out together: if one has words left, they do
         * not match. */
        if (a[i] == '\0' || b[j] == '\0') {
            return a[i] == '\0' && b[j] == '\0';
        }
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[j])) {
            return 0;
        }
        i++;
        j++;
    }
}

/* A clock for timing practice, in seconds. The monotonic clock keeps going
 * even if the system clock is changed underneath us. */
static double seconds_now(void) {
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return (double)time(NULL);
    }
    return (double)now.tv_sec + (double)now.tv_nsec / 1000000000.0;
}

/* Something gentle to say when the answer is not quite right. */
static const char *gentle_nudge(void) {
    static const char *nudges[] = {
        "Not quite, have another go!",
        "Nearly! Tricky one, isn't it? Try again.",
        "Oh dear, that one got away. Go again!",
        "So close! Have another go, you are nearly there.",
        "Nearly there! Read it once more, then try again."
    };

    return nudges[(size_t)rand() % (sizeof(nudges) / sizeof(nudges[0]))];
}

/* Show a twister and let the user type it back, over and over, getting
 * quicker. An empty line finishes. */
static void practice_mode(const char *twister) {
    double best = 0.0; /* quickest time so far, 0 means none yet */

    printf("%s\n", twister);
    printf("Type it back and press Enter. An empty line finishes.\n");

    for (;;) {
        double start = seconds_now();
        double took;
        char *answer = read_answer();

        if (answer == NULL) {
            printf("\nCheerio! Do come back and practise again.\n");
            return;
        }
        if (answer[0] == '\0') {
            printf("Cheerio! Keep those vowels warm.\n");
            free(answer);
            return;
        }

        took = seconds_now() - start;
        if (same_words(twister, answer)) {
            printf("Brilliant! You said it in %.1f seconds.", took);
            if (best == 0.0 || took < best) {
                if (best > 0.0) {
                    printf(" A new best!");
                }
                best = took;
            }
            printf("\nLovely stuff. Have another go, quicker this time.\n\n");
        } else {
            printf("%s\n\n", gentle_nudge());
            /* Show the twister again so it is on screen for the next go. */
            printf("%s\n", twister);
        }
        free(answer);
    }
}

static void print_help(void) {
    printf("Usage: lingua [options]\n");
    printf("\n");
    printf("  lingua                 a random tongue twister (new every time)\n");
    printf("  lingua -p              a random twister, then practise it\n");
    printf("  lingua -f              a funny joke about speech or language\n");
    printf("  lingua -h              this help\n");
    printf("  lingua --twisters FILE use a different twisters file\n");
    printf("  lingua --jokes FILE    use a different jokes file\n");
    printf("\n");
    printf("Add twisters to twisters.txt and jokes to jokes.txt, with a\n");
    printf("line holding just %% between them. No rebuild needed.\n");
}

/* A file name that has to follow an option. */
static const char *file_after_option(const char *option, int argc,
                                     char **argv, int *index) {
    if (*index + 1 >= argc) {
        fprintf(stderr, "Sorry, %s needs a file name after it.\n", option);
        return NULL;
    }
    *index += 1;
    return argv[*index];
}

int main(int argc, char **argv) {
    int practise = 0;
    int funny = 0;
    const char *twisters_file = NULL;
    const char *jokes_file = NULL;
    char *text;
    int i;

    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            print_help();
            return 0;
        } else if (strcmp(arg, "-p") == 0) {
            practise = 1;
        } else if (strcmp(arg, "-f") == 0) {
            funny = 1;
        } else if (strcmp(arg, "--twisters") == 0) {
            twisters_file = file_after_option("--twisters", argc, argv, &i);
            if (twisters_file == NULL) {
                return 1;
            }
        } else if (strcmp(arg, "--jokes") == 0) {
            jokes_file = file_after_option("--jokes", argc, argv, &i);
            if (jokes_file == NULL) {
                return 1;
            }
        } else {
            fprintf(stderr, "Sorry, I do not know '%s'.\n", arg);
            print_help();
            return 1;
        }
    }

    if (funny && practise) {
        fprintf(stderr,
                "A joke and a twister in one breath? Brilliant, but I\n");
        fprintf(stderr,
                "can only do one at a time. Try -f or -p on its own.\n");
        return 1;
    }

    seed_random();

    if (funny) {
        text = pick_one(jokes_file, "jokes.txt", "joke", "last-joke", 1);
        if (text == NULL) {
            return 1;
        }
        printf("%s\n", text);
        free(text);
        return 0;
    }

    /* Practice mode picks freely: an empty line should not turn into the
     * same twister again. */
    text = pick_one(twisters_file, "twisters.txt", "twister", "last-twister",
                    !practise);
    if (text == NULL) {
        return 1;
    }

    if (practise) {
        practice_mode(text);
    } else {
        printf("%s\n", text);
        printf("Have a go three times, then try it twice as fast.\n");
    }
    free(text);
    return 0;
}