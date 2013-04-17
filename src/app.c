
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ltsv4c.h"

#define BUFFER_SIZE 4096

#define print_line(color, label, value) ((color) ?  printf("%c[1;34m%s%c[0m: %c[32m%s%c[0m\n", 27, label, 27, 27, value, 27) : printf("%s: %s\n", label, value))

typedef struct {
    char **values;
    size_t count;
} label_names_t;

static label_names_t *
_names_init(char *label)
{
    label_names_t *self = NULL;
    char *value, *str;
    size_t i, len;

    self = (label_names_t *)malloc(sizeof(label_names_t));
    if (!self) {
        fprintf(stderr, "ERR: Memory allocate in label names.\n");
        return NULL;
    }

    self->count = 0;

    value = label;
    while (1) {
        self->count++;
        str = strchr(value, ',');
        if (str == NULL) {
            break;
        }
        value = (++str);
    }

    if (self->count == 0) {
        free(self);
        return NULL;
    }

    self->values = (char **)malloc(sizeof(char *) * self->count);
    if (!self->values) {
        fprintf(stderr, "ERR: Memory allocate in label names value.\n");
        free(self);
        return NULL;
    }

    value = label;
    for (i = 0; i < self->count; i++) {
        self->values[i] = NULL;

        str = strtok(value, ",");
        if (str) {
            len = strlen(str) + 1;
            self->values[i] = (char *)malloc(sizeof(char) * len);
            if (self->values[i]) {
                strcpy(self->values[i], str);
            } else {
                fprintf(stderr, "ERR: Memory allocate in label name.\n");
            }
        }
        value = NULL;
    }

    return self;
}

static void
_names_destroy(label_names_t *self)
{
    if (self) {
        size_t i;
        for (i = 0; i < self->count; i++) {
            if (self->values[i]) {
                free(self->values[i]);
            }
        }
        free(self->values);
        free(self);
    }
}

static int
_stdin(void) {
    struct stat st;

    if (fstat(STDIN_FILENO, &st) == 0 &&
        S_ISFIFO(st.st_mode)) {
        return 1;
    }

    return 0;
}

static int
_ltsv_print(LTSV *ltsv, int color, int verbose, label_names_t *names)
{
    size_t i, count;

    if (!ltsv) {
        return -1;
    }

    count = ltsv_get_count(ltsv);
    if (count == 0) {
        fprintf(stderr, "ERR: ltsv count doesn't\n");
        return -1;
    }

    for (i = 0; i < count; i++) {
        int output = 0;
        size_t index;
        LTSV_Record *record = ltsv_get_record(ltsv, i);
        if (!record) {
            continue;
        }

        if (names) {
            for (index = 0; index < names->count; index++) {
                char *name = names->values[index];
                const char *value = NULL;
                if (name) {
                    value = ltsv_record_get_value(record, name);
                }
                if (verbose || value) {
                    print_line(color, name, value);
                    output = 1;
                }
            }
        } else {
            size_t n = ltsv_record_get_count(record);
            for (index = 0; index < n; index++) {
                const char *name = ltsv_record_get_name(record, index);
                const char *value = NULL;
                if (name) {
                    value = ltsv_record_get_value(record, name);
                }
                if (verbose || value) {
                    print_line(color, name, value);
                    output = 1;
                }
            }
        }
        if (output) {
            printf("--\n");
        }
    }

    return 0;
}

static void
_usage(char *arg, char *message)
{
    char *command = basename(arg);

    printf("Usage: %s [OPTIONS]\n\n", command);
    printf("OPTIONS:\n");
    printf("  --file    [ -f ] <filename>          input file name.\n");
    printf("  --label   [ -l ] <name1[,name2 ...]> label names.\n");
    printf("  --buffer  [ -b ] <size>              read line buffer size.\n");
    printf("  --color   [ -c ]                     output ansi color.\n");
    printf("  --verbose [ -v ]                     output null value.\n");

    if (message) {
        printf("\nINFO: %s\n", message);
    }
}

int
main(int argc, char **argv)
{
    int status = 0, color = 0, verbose = 0;
    size_t size = BUFFER_SIZE;
    char *filename = NULL, *label = NULL;

    LTSV *ltsv = NULL;
    label_names_t *names = NULL;

    int opt;
    const struct option long_options[] = {
        { "file", 1, NULL, 'f' },
        { "label", 1, NULL, 'l' },
        { "buffer", 1, NULL, 'b' },
        { "color", 0, NULL, 'c' },
        { "verbose", 0, NULL, 'v' },
        { "help", 0, NULL, 'h' },
        { NULL, 0, NULL, 0 }
    };

    while ((opt = getopt_long_only(argc, argv, "", long_options, NULL)) != -1) {
        switch (opt) {
            case 'f':
                filename = optarg;
                break;
            case 'l':
                label = optarg;
                break;
            case 'b':
                size = atol(optarg);
                break;
            case 'c':
                color = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                _usage(argv[0], NULL);
                return -1;
        }
    }

    if (label) {
        names = _names_init(label);
    }

    if (filename != NULL) {
        //file
        printf("--\n");
        ltsv = ltsv_parse_file(filename);
        if (ltsv) {
            status = _ltsv_print(ltsv, color, verbose, names);
            ltsv_free(ltsv);
        }
    } else if (_stdin()) {
        //stdin
        printf("--\n");
        if (size == 0) {
            size = BUFFER_SIZE;
        }
        while (1) {
            char buf[size+1];
            size_t len;

            memset(buf, '\0', size+1);
            len = read(0, buf, size);
            if (len == 0) {
                break;
            }

            ltsv = ltsv_parse_string(buf);
            if (ltsv) {
                status = _ltsv_print(ltsv, color, verbose, names);
                ltsv_free(ltsv);
            }
            if (status != 0) {
                break;
            }
        }
    } else {
        _usage(argv[0], "Input -f <filename> or stdin string.\n");
    }

    if (names) {
        _names_destroy(names);
    }

    return 0;
}
