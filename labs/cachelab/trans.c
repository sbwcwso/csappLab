/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include "cachelab.h"
#include <stdio.h>

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */

char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    register int i, j, ii, jj;
    register int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    if (M == 32 && N == 32)
    {
        for (i = 0; i < M; i += 8)
        {
            // 1. first handle the main diagonal block on ecah row
            // if i != 0
            // use B_{i/8}{0} as the cache block for the main diagonal block B_{i/8}{i/8}
            // if i == 0
            // use B_{0}{1} as the cache block for the main diagonal block B_{0}{0}
            j = i ? 0 : 8;

            // 1.1 store the main diagonal block at the cache block
            for (ii = i; ii < i + 8; ii++)
                for (jj = i; jj < i + 8; jj++)
                    B[ii][jj - i + j] = A[jj][ii];

            // 1.2 copy the item in the cache block to the main block
            for (ii = i; ii < i + 8; ii++)
                for (jj = i; jj < i + 8; jj++)
                    B[ii][jj] = B[ii][jj - i + j];

            // 2. Handle the block does not on the main diagonal
            for (j = 0; j < N; j += 8)
                if (i != j)
                    for (ii = i; ii < i + 8; ii++)
                        for (jj = j; jj < j + 8; jj++)
                            B[ii][jj] = A[jj][ii];
        }
    }
    else if (M == 64 && N == 64)
    {
        for (i = 0; i < M; i += 8)
        {
            // 1. handle the block on the main diagonal
            // for B_{0}{0}, selct C from first 4 lines from B_{0}{1}, selct D from frist 4 lines from B_{0}{2}
            // for B_{1}{0}, selct C from first 4 lines from B_{1}{0}, selct D from frist 4 lines from B_{1}{2}
            // for other B_{i}{i}, selct C from first 4 lines from B_{i}{0}, selct D from frist 4 lines from B_{i}{1}
            tmp0 = (i == 0) ? 8 : 0;
            tmp1 = (i > 8) ? 8 : 16;


            // C = [A11^T, A22^T], D = [A12^T, A22^T]
            for (ii = 0; ii < 8; ii++)
                for (jj = 0; jj < 4; jj++)
                {
                    B[i + jj][tmp0 + ii] = A[i + ii][i + jj];
                    B[i + jj][tmp1 + ii] = A[i + ii][i + jj + 4];
                }

            // B = [ C
            //       D ]
            for (ii = 0; ii < 4; ii++)
            {
                for (jj = 0; jj < 8; jj++)
                    B[i + ii][i + jj] = B[i + ii][tmp0 + jj];
                for (jj = 0; jj < 8; jj++)
                    B[i + ii + 4][i + jj] = B[i + ii][tmp1 + jj];
            }

            // 2. handle the block not on the main diagonal
            for (j = 0; j < N; j += 8)
            {
                if (i == j)
                    continue;

                // B = [ A11^T, A12^T
                //       N21, N22 ]
                for (ii = 0; ii < 4; ii++)
                {
                    for (jj = 0; jj < 4; jj++)
                    {
                        B[i + jj][j + ii] = A[j + ii][i + jj];
                        B[i + jj][j + ii + 4] = A[j + ii][i + jj + 4];
                    }
                }

                for (ii = 0; ii < 4; ii++)
                {
                    tmp2 = B[i + ii][j + 4];
                    tmp3 = B[i + ii][j + 5];
                    tmp4 = B[i + ii][j + 6];
                    tmp5 = B[i + ii][j + 7];

                    // A12^T to A21^T
                    for (jj = 4; jj < 8; jj++)
                    {
                        B[i + ii][j + jj] = A[j + jj][i + ii];
                    }

                    // N21 to A12^T
                    B[i + 4 + ii][j] = tmp2;
                    B[i + 4 + ii][j + 1] = tmp3;
                    B[i + 4 + ii][j + 2] = tmp4;
                    B[i + 4 + ii][j + 3] = tmp5;

                    // N22 to A22^T
                    for (jj = 4; jj < 8; jj++)
                    {
                        B[i + 4 + ii][j + jj] = A[j + jj][i + ii + 4];
                    }
                }
            }
        }
    }
    else if (M == 61 && N == 67)
    {
        // 8 * 8 blocks
        for (i = 0; i < 56; i += 8)
        {
            for (j = 0; j < 64; j += 8)
                for (ii = 0; ii < 8; ii++)
                {
                    tmp0 = A[j + ii][i];
                    tmp1 = A[j + ii][i + 1];
                    tmp2 = A[j + ii][i + 2];
                    tmp3 = A[j + ii][i + 3];
                    tmp4 = A[j + ii][i + 4];
                    tmp5 = A[j + ii][i + 5];
                    tmp6 = A[j + ii][i + 6];
                    tmp7 = A[j + ii][i + 7];

                    B[i][j + ii] = tmp0;
                    B[i + 1][j + ii] = tmp1;
                    B[i + 2][j + ii] = tmp2;
                    B[i + 3][j + ii] = tmp3;
                    B[i + 4][j + ii] = tmp4;
                    B[i + 5][j + ii] = tmp5;
                    B[i + 6][j + ii] = tmp6;
                    B[i + 7][j + ii] = tmp7;
                }
            // 3 * 8 blocks
            for (; j < 67; j++)
            {
                for (ii = 0; ii < 8; ii++)
                {
                    B[i + ii][j] = A[j][i + ii];
                }
            }
        }

        // 5 * 8 blocks
        for (j = 0; j < 64; j += 8)
        {
            for (ii = j; ii < j + 8; ii++)
            {
                tmp0 = A[ii][i];
                tmp1 = A[ii][i + 1];
                tmp2 = A[ii][i + 2];
                tmp3 = A[ii][i + 3];
                tmp4 = A[ii][i + 4];

                B[i][ii] = tmp0;
                B[i + 1][ii] = tmp1;
                B[i + 2][ii] = tmp2;
                B[i + 3][ii] = tmp3;
                B[i + 4][ii] = tmp4;
            }
        }

        //  the last 5 * 3 block
        for (ii = 64; ii < 67; ii++)
        {
            tmp0 = A[ii][i];
            tmp1 = A[ii][i + 1];
            tmp2 = A[ii][i + 2];
            tmp3 = A[ii][i + 3];
            tmp4 = A[ii][i + 4];

            B[i][ii] = tmp0;
            B[i + 1][ii] = tmp1;
            B[i + 2][ii] = tmp2;
            B[i + 3][ii] = tmp3;
            B[i + 4][ii] = tmp4;
        }
    }
}
/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
// with the original operation (without blocks-shifting and lazy-transposing), 1176 = 35 * 8 + 16 * 56
// with block-shifting and lazy-transposing, it reaches theoratical limit: 1024 = 16 * 64

void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; ++j)
        {
            if (A[i][j] != B[j][i])
            {
                return 0;
            }
        }
    }
    return 1;
}
