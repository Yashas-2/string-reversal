/*
 * Parallel String Reversal Using OpenMP
 *
 * Academic mini-project for Parallel Programming demonstration.
 *
 * This program reverses a string using two methods:
 *   1. Sequential: a single loop runs on one core.
 *   2. Parallel:   OpenMP splits the same loop across many threads.
 *
 * How the reversal works
 * ----------------------
 * To reverse a string of length n, we swap character pairs:
 *      s[0]   <-> s[n-1]
 *      s[1]   <-> s[n-2]
 *      ...
 *      s[i]   <-> s[n-1-i]     for i = 0 .. (n/2)-1
 *
 * Every swap touches a different pair of positions, so the
 * iterations are independent. That is why they can safely run
 * in parallel with #pragma omp parallel for.
 *
 * Usage (text is read from standard input, one line):
 *   reversal sequential          -> run sequential reverse only
 *   reversal parallel <threads>  -> run parallel reverse only
 *
 * Output is a single JSON object on stdout.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define MAX_INPUT 2000000   /* 2 million characters max */

/* ------------------------------------------------------------------ */
/* Sequential reversal                                                 */
/* One thread does every swap, one after another.                      */
/* ------------------------------------------------------------------ */
static void reverse_sequential(char *s, long long n) {
    for (long long i = 0; i < n / 2; ++i) {
        /* Swap the pair (i, n-1-i). Each iteration is independent. */
        char temp      = s[i];
        s[i]           = s[n - 1 - i];
        s[n - 1 - i]   = temp;
    }
}

/* ------------------------------------------------------------------ */
/* Parallel reversal with OpenMP                                       */
/* The same loop, but #pragma omp parallel for tells the compiler      */
/* to distribute loop iterations across several threads.               */
/* Each thread swaps its own pairs -- no locks needed.                 */
/* ------------------------------------------------------------------ */
static void reverse_parallel(char *s, long long n, int threads) {
    #pragma omp parallel for num_threads(threads) schedule(static)
    for (long long i = 0; i < n / 2; ++i) {
        char temp      = s[i];
        s[i]           = s[n - 1 - i];
        s[n - 1 - i]   = temp;
    }
}

/* ------------------------------------------------------------------ */
/* Print a C string as a JSON string (escape special characters).      */
/* ------------------------------------------------------------------ */
static void print_json_string(const char *s) {
    putchar('"');
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
        switch (*p) {
            case '"':  printf("\\\""); break;
            case '\\': printf("\\\\");  break;
            case '\n': printf("\\n");   break;
            case '\r': printf("\\r");   break;
            case '\t': printf("\\t");   break;
            default:
                if (*p < 0x20) printf("\\u%04x", *p);
                else           putchar(*p);
        }
    }
    putchar('"');
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */
int main(int argc, char **argv) {
    int threads = 1;
    int run_parallel = 0;

    /* Decide which algorithm to run based on command-line arguments. */
    if (argc >= 2 && strcmp(argv[1], "parallel") == 0) {
        run_parallel = 1;
        if (argc >= 3) threads = atoi(argv[2]);
        if (threads < 1 || threads > 64) {
            fprintf(stderr, "Thread count must be between 1 and 64.\n");
            return 2;
        }
    } else if (argc >= 2 && strcmp(argv[1], "sequential") != 0) {
        fprintf(stderr, "Usage: %s sequential | %s parallel <threads>\n", argv[0], argv[0]);
        return 2;
    }

    /* Read one line of input. */
    char *input = (char *)malloc(MAX_INPUT + 1);
    if (!input) { fprintf(stderr, "Out of memory.\n"); return 3; }

    if (!fgets(input, MAX_INPUT + 1, stdin)) {
        /* Empty input is allowed; treat as empty string. */
        input[0] = '\0';
    }

    /* Strip trailing newline / carriage return. */
    size_t len = strlen(input);
    while (len > 0 && (input[len - 1] == '\n' || input[len - 1] == '\r')) {
        input[--len] = '\0';
    }

    /* Work on a copy so the original is preserved for output. */
    char *work = (char *)malloc(len + 1);
    if (!work) { free(input); fprintf(stderr, "Out of memory.\n"); return 3; }
    memcpy(work, input, len + 1);

    /* ---- measure execution time with omp_get_wtime() ---- */
    double start, elapsed;

    start = omp_get_wtime();
    if (run_parallel) {
        reverse_parallel(work, (long long)len, threads);
    } else {
        reverse_sequential(work, (long long)len);
    }
    elapsed = omp_get_wtime() - start;

    /* If the timer returned a tiny negative value (clock adjustment), clamp to 0. */
    if (elapsed < 0.0) elapsed = 0.0;

    /* ---- correctness check ----
     * Reverse the result again; it must match the original string. */
    char *check = (char *)malloc(len + 1);
    int correct = 1;
    if (check) {
        memcpy(check, work, len + 1);
        reverse_sequential(check, (long long)len);
        correct = (strcmp(check, input) == 0);
        free(check);
    }

    /* ---- output JSON for the Flask backend ---- */
    printf("{\"original\":");   print_json_string(input);
    printf(",\"reversed\":");   print_json_string(work);
    printf(",\"length\":%lu", (unsigned long)len);
    printf(",\"mode\":\"%s\"", run_parallel ? "parallel" : "sequential");
    printf(",\"threads\":%d", run_parallel ? threads : 1);
    printf(",\"time_seconds\":%.12f", elapsed);
    printf(",\"correct\":%s}\n", correct ? "true" : "false");

    free(input);
    free(work);
    return 0;
}
