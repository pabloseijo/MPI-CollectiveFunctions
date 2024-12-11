#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

void imprimir_matriz(double* matrix, int filas, int columnas, const char* nombre, int rank);

int main(int argc, char* argv[]) {
    int rank, tam, N, C_max;
    double lambda;
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

        if (rank == 0) {
            for (int p = 0; p < tam; p++) {
                int start_col = columnas_procesadas + p * cols_to_send;
                if (start_col < N) {
                    MPI_Send(&cols_to_send, 1, MPI_INT, p, 0, MPI_COMM_WORLD);
                    MPI_Send(&A[start_col * N], cols_to_send * N, MPI_DOUBLE, p, 1, MPI_COMM_WORLD);
                    MPI_Send(&B[start_col * N], cols_to_send * N, MPI_DOUBLE, p, 2, MPI_COMM_WORLD);
                } else {
                    int zero = 0;
                    MPI_Send(&zero, 1, MPI_INT, p, 0, MPI_COMM_WORLD);
                }
            }
        }

        // Recepción del número de columnas
        int cols_received;
        MPI_Recv(&cols_received, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (cols_received > 0) {
            local_A = (double*)malloc(N * cols_received * sizeof(double));
            local_B = (double*)malloc(N * cols_received * sizeof(double));
            local_C = (double*)malloc(N * cols_received * sizeof(double));

            MPI_Recv(local_A, N * cols_received, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(local_B, N * cols_received, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Imprimir matrices locales
            printf("\nProceso %d - local_A:\n", rank);
            imprimir_matriz(local_A, N, cols_received, "local_A", rank);
            printf("\nProceso %d - local_B:\n", rank);
            imprimir_matriz(local_B, N, cols_received, "local_B", rank);

            // Calcular local_C
            for (int col = 0; col < cols_received; col++) {
                for (int row = 0; row < N; row++) {
                    local_C[row * cols_received + col] =
                        lambda * local_A[row * cols_received + col] + local_B[row * cols_received + col];
                }
            }

            MPI_Send(local_C, N * cols_received, MPI_DOUBLE, 0, 3, MPI_COMM_WORLD);

            free(local_A);
            free(local_B);
            free(local_C);
        }

        if (rank == 0) {
            for (int p = 0; p < tam; p++) {
                int start_col = columnas_procesadas + p * cols_to_send;
                if (start_col < N) {
                    MPI_Recv(&C[start_col * N], cols_to_send * N, MPI_DOUBLE, p, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            }
        }

        columnas_procesadas += cols_to_send * tam;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        printf("\nMatriz C resultado final:\n");
        imprimir_matriz(C, N, N, "C", rank);
        free(A);
        free(B);
        free(C);
    }

    MPI_Finalize();
    return 0;
}

void imprimir_matriz(double* matrix, int filas, int columnas, const char* nombre, int rank) {
    printf("Proceso %d - Matriz %s:\n", rank, nombre);
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            printf("%6.2lf ", matrix[i * columnas + j]);
        }
        printf("\n");
    }
}
