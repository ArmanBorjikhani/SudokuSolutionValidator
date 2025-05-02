/*==============================================
  Multithreaded Sudoku solution validator
  Build:  gcc -pthread -o sudoku_validator sudoku_validator.c
  Usage:  ./sudoku_validator <input_file>
          Input file must contain 81 integers (0‑9) separated by white‑space.
          Any non‑zero integer outside 1‑9 causes failure.
  ==============================================*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#define SIZE 9

int sudoku[SIZE][SIZE];            /* the grid under test  */
int results[11] = {0};             /* validity flags for each worker */

/*----------------------------------------------
  Thread 0: validate all rows
----------------------------------------------*/
void *check_rows(void *arg)
{
    (void)arg;
    for (int r = 0; r < SIZE; ++r) {
        bool seen[SIZE + 1] = {false};
        for (int c = 0; c < SIZE; ++c) {
            int val = sudoku[r][c];
            if (val < 1 || val > 9 || seen[val]) {
                pthread_exit(NULL);
            }
            seen[val] = true;
        }
    }
    results[0] = 1;
    pthread_exit(NULL);
}

/*----------------------------------------------
  Thread 1: validate all columns
----------------------------------------------*/
void *check_cols(void *arg)
{
    (void)arg;
    for (int c = 0; c < SIZE; ++c) {
        bool seen[SIZE + 1] = {false};
        for (int r = 0; r < SIZE; ++r) {
            int val = sudoku[r][c];
            if (val < 1 || val > 9 || seen[val]) {
                pthread_exit(NULL);
            }
            seen[val] = true;
        }
    }
    results[1] = 1;
    pthread_exit(NULL);
}

/*----------------------------------------------
  Thread 2‑10: validate individual 3×3 subgrids
  idx 0..8 maps to subgrids row_block = idx/3, col_block = idx%3
----------------------------------------------*/
void *check_subgrid(void *arg)
{
    int idx = *(int *)arg; /* 0..8 */
    int row_start = (idx / 3) * 3;
    int col_start = (idx % 3) * 3;

    bool seen[SIZE + 1] = {false};
    for (int r = row_start; r < row_start + 3; ++r) {
        for (int c = col_start; c < col_start + 3; ++c) {
            int val = sudoku[r][c];
            if (val < 1 || val > 9 || seen[val]) {
                pthread_exit(NULL);
            }
            seen[val] = true;
        }
    }
    results[2 + idx] = 1;
    pthread_exit(NULL);
}

/*--------------- helper to load puzzle ----------------*/
static void load_or_die(const char *file)
{
    FILE *fp = fopen(file, "r");
    if (!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    for (int r = 0; r < SIZE; ++r)
        for (int c = 0; c < SIZE; ++c)
            if (fscanf(fp, "%d", &sudoku[r][c]) != 1) {
                fprintf(stderr, "Invalid input file.\n");
                exit(EXIT_FAILURE);
            }
    fclose(fp);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <sudoku_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    load_or_die(argv[1]);

    pthread_t tid[11];
    int subgrid_idx[9];

    /* launch validator threads */
    pthread_create(&tid[0], NULL, check_rows, NULL);
    pthread_create(&tid[1], NULL, check_cols, NULL);
    for (int i = 0; i < 9; ++i) {
        subgrid_idx[i] = i;
        pthread_create(&tid[2 + i], NULL, check_subgrid, &subgrid_idx[i]);
    }

    /* wait for all */
    for (int i = 0; i < 11; ++i)
        pthread_join(tid[i], NULL);

    /* evaluate */
    bool ok = true;
    for (int i = 0; i < 11; ++i)
        ok &= results[i];

    printf("Sudoku is %s\n", ok ? "VALID" : "INVALID");
    return 0;
}