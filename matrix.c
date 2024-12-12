#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

void imprimir_matriz(double* matrix, int filas, int columnas, const char* nombre, int rank);

int main(int argc, char* argv[]) {
    int rank, tam, N, C_max;
    double lambda, start_time, end_time;
    double *A = NULL, *B = NULL, *C = NULL;

    double *local_A = NULL, *local_B = NULL, *local_C = NULL;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &tam);

    if (argc != 4) {
        if (rank == 0) {
            fprintf(stderr, "Uso: %s <Tamaño de la matriz N> <Valor de lambda> <C_max>\n", argv[0]);
        }
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    N = atoi(argv[1]);
    lambda = atof(argv[2]);
    C_max = atoi(argv[3]);

    // Medición de tiempo
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    // Tamaños y desplazamientos para Scatterv y Gatherv
    int *sendcounts = malloc(tam * sizeof(int));
    int *displs = malloc(tam * sizeof(int));

    if (rank == 0) {
        A = (double*)malloc(N * N * sizeof(double));
        B = (double*)malloc(N * N * sizeof(double));
        C = (double*)calloc(N * N, sizeof(double));

        // Inicializar matrices A y B
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                A[i * N + j] = i + j;
                B[i * N + j] = i - j;
            }
        }
    }

    int columnas_procesadas = 0;

    while (columnas_procesadas < N) {
        int cols_to_send = (C_max < (N - columnas_procesadas)) ? C_max : (N - columnas_procesadas);

        // Calcular sendcounts y displs de manera cíclica
        for (int p = 0; p < tam; p++) {
            sendcounts[p] = (p * cols_to_send + columnas_procesadas < N) ? N * cols_to_send : 0;
            displs[p] = (p * cols_to_send + columnas_procesadas) * N;
        }

        int local_col_count = sendcounts[rank] / N;

        local_A = (double*)malloc(N * local_col_count * sizeof(double));
        local_B = (double*)malloc(N * local_col_count * sizeof(double));
        local_C = (double*)malloc(N * local_col_count * sizeof(double));

        // Distribuir columnas con MPI_Scatterv
        MPI_Scatterv(A, sendcounts, displs, MPI_DOUBLE, local_A, N * local_col_count, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Scatterv(B, sendcounts, displs, MPI_DOUBLE, local_B, N * local_col_count, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // Calcular local_C
        for (int col = 0; col < local_col_count; col++) {
            for (int row = 0; row < N; row++) {
                local_C[row * local_col_count + col] =
                    lambda * local_A[row * local_col_count + col] + local_B[row * local_col_count + col];
            }
        }

        // Recolectar resultados con MPI_Gatherv
        MPI_Gatherv(local_C, N * local_col_count, MPI_DOUBLE, C, sendcounts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        free(local_A);
        free(local_B);
        free(local_C);

        columnas_procesadas += cols_to_send * tam;  // Actualizar columnas procesadas
    }

    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();

    // Imprimir tiempo solo en el proceso 0
    if (rank == 0) {
        printf("Tiempo total de ejecución: %f segundos con %d procesos y tamaño N=%d.\n", end_time - start_time, tam, N);
        free(A);
        free(B);
        free(C);
    }

    free(sendcounts);
    free(displs);
    MPI_Finalize();
    return 0;
}

void imprimir_matriz(double* matrix, int filas, int columnas, const char* nombre, int rank) {
    printf("Proceso %d - Matriz %s:\n", rank, nombre);
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            printf("%lf ", matrix[i * columnas + j]);
            printf("%6.2lf ", matrix[i * columnas + j]);
        }
        printf("\n");
    }
}