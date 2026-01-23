/*
 * Parallel Matrix Multiplication using pi_cl_team_fork
 * Each core computes a chunk of the output matrix rows
 */

#include "pmsis.h"
#include "stdio.h"
#include "pmsis/cluster/cluster_team/cl_team.h"

// Matrix dimensions
#define N 128 // Matrix size NxN

// Global matrices in L1 memory (shared by all cluster cores)
static int A[N * N];
static int B[N * N];
static int C[N * N];
static int errors = 0;

// Initialize matrices with simple patterns
static void init_matrices()
{
    for (int i = 0; i < N * N; i++)
    {
        A[i] = i % 10;       // 0-9 pattern
        B[i] = (i + 1) % 10; // 1-10 pattern
        C[i] = 0;            // Zero output
    }
}

// Each core computes its assigned rows
static void matmul_kernel(void *arg)
{
    int core_id = pi_core_id();
    int num_cores = pi_cl_cluster_nb_cores();

    // Divide rows among cores
    int rows_per_core = N / num_cores;
    int start_row = core_id * rows_per_core;
    int end_row = (core_id == num_cores - 1) ? N : start_row + rows_per_core;

    // Compute assigned rows: C[i,j] = sum(A[i,k] * B[k,j])
    for (int i = start_row; i < end_row; i++)
    {
        for (int j = 0; j < N; j++)
        {
            int sum = 0;
            for (int k = 0; k < N; k++)
            {
                sum += A[i * N + k] * B[k * N + j];
            }
            C[i * N + j] = sum;
        }
    }
}

// Verify result (simple check on diagonal)
static void verify_result()
{
    printf("\nVerifying results...\n");
    int expected = 0;

    // Calculate expected value for C[0,0]
    for (int k = 0; k < N; k++)
    {
        expected += A[k] * B[k * N];
    }

    if (C[0] == expected)
    {
        printf("✓ C[0,0] = %d (correct)\n", C[0]);
    }
    else
    {
        printf("✗ C[0,0] = %d (expected %d)\n", C[0], expected);
        errors++;
    }

    // Print sample results
    printf("\nSample results (first 4x4 submatrix of C):\n");
    for (int i = 0; i < 4 && i < N; i++)
    {
        for (int j = 0; j < 4 && j < N; j++)
        {
            printf("%5d ", C[i * N + j]);
        }
        printf("\n");
    }
}

// Cluster entry point
static void cluster_entry(void *arg)
{
    int *err = (int *)arg;
    int num_cores = pi_cl_cluster_nb_cores();

    printf("[Cluster] Starting parallel matmul with %d cores\n", num_cores);
    printf("[Cluster] Computing %dx%d matrix multiplication\n", N, N);

    // Initialize performance counters
    pi_perf_conf(
        1 << PI_PERF_CYCLES |
        1 << PI_PERF_INSTR);

    pi_perf_reset();
    pi_perf_start();

    // Fork computation across all cores
    pi_cl_team_fork(num_cores, matmul_kernel, NULL);

    pi_perf_stop();

    uint32_t cycles = pi_perf_read(PI_PERF_CYCLES);
    uint32_t instr = pi_perf_read(PI_PERF_INSTR);

    printf("[Cluster] Computation complete\n");
    printf("[Cluster] Cycles: %u, Instructions: %u, CPI: %.2f\n",
           cycles, instr, (float)cycles / instr);

    // Verify on core 0
    if (pi_core_id() == 0)
    {
        verify_result();
    }

    *err = errors;
}

// Main function running on fabric controller
int test_entry()
{
    struct pi_device cluster_dev;
    struct pi_cluster_conf conf;
    struct pi_cluster_task task;
    int test_errors = 0;

    printf("\n=== Parallel Matrix Multiplication Test ===\n");
    printf("Matrix size: %dx%d\n", N, N);

    // Initialize matrices
    init_matrices();
    printf("Matrices initialized\n");

    // Configure and open cluster
    pi_cluster_conf_init(&conf);
    conf.id = 0;

    pi_open_from_conf(&cluster_dev, &conf);
    pi_cluster_open(&cluster_dev);

    // Create cluster task
    pi_cluster_task(&task, cluster_entry, (void *)&test_errors);

    // Send task to cluster
    pi_cluster_send_task_to_cl(&cluster_dev, &task);

    // Close cluster
    pi_cluster_close(&cluster_dev);

    return test_errors;
}

void test_kickoff(void *arg)
{
    int ret = test_entry();
    if (ret)
        printf("\n✗ Test FAILED with %d errors\n", ret);
    else
        printf("\n✓ Test PASSED\n");
    pmsis_exit(ret);
}

int main()
{
    return pmsis_kickoff((void *)test_kickoff);
}
