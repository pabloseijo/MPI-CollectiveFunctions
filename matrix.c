#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

void guardar_matriz(double* matrix, int N, const char* filename, int rank, int tam);
void imprimir_matriz(double* matrix, int filas, int columnas, const char* nombre, int rank);

int main(int argc, char* argv[]) {
    int rank, tam, N;
    double lambda;
    double *A = NULL, *B = NULL, *C = NULL;

    double *local_A = NULL, *local_B = NULL, *local_C = NULL;
    int *sendcounts = NULL, *displs = NULL;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &tam);

    if (argc != 3) {
        if (rank == 0) {
            fprintf(stderr, "Uso: %s <Tamaño de la matriz N> <Valor de lambda>\n", argv[0]);
        }
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    N = atoi(argv[1]);
    lambda = atof(argv[2]);

      /**
     * Para solucionar el problema de la distribución por columnas vamos a tener lo siguiente: Primero 
     * necesitamos un array para saber cuántas columnas le corresponde a cada proceso y otro array para
     * para saber en que posición se encuentra cada elemento de la columna tal que podemos ir saltando las
     * posiciones de memoria en función de la columna que le corresponde a cada proceso. Es decir, tenemos
     * la columna 1, pues saltamos tantas posiciones de memoria como elementos tenga la fila para coger el primer 
     * elemento de cada array consecutivo en memoria
    */

    // Calcular sendcounts y displs correctamente
    sendcounts = (int*)malloc(tam * sizeof(int));
    displs = (int*)malloc(tam * sizeof(int));
    for (int p = 0; p < tam; p++) {
        sendcounts[p] = 0;
        for (int j = p; j < N; j += tam) {
            sendcounts[p] += N;  // Número total de elementos para las columnas asignadas
        }
        displs[p] = (p == 0) ? 0 : displs[p - 1] + sendcounts[p - 1];
    }

    int local_col_count = sendcounts[rank] / N;  // Número de columnas locales
    local_A = (double*)malloc(N * local_col_count * sizeof(double));
    local_B = (double*)malloc(N * local_col_count * sizeof(double));
    local_C = (double*)malloc(N * local_col_count * sizeof(double));

    if (rank == 0) {
        A = (double*)malloc(N * N * sizeof(double));
        B = (double*)malloc(N * N * sizeof(double));
        C = (double*)malloc(N * N * sizeof(double));

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                A[i * N + j] = i + j;
                B[i * N + j] = i - j;
            }
        }

        printf("\nMatriz A inicial:\n");
        imprimir_matriz(A, N, N, "A", rank);
        printf("\nMatriz B inicial:\n");
        imprimir_matriz(B, N, N, "B", rank);
    }

    // Distribuir columnas cíclicas de A y B
    MPI_Scatterv(A, sendcounts, displs, MPI_DOUBLE, local_A, sendcounts[rank], MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatterv(B, sendcounts, displs, MPI_DOUBLE, local_B, sendcounts[rank], MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Imprimir matrices locales
    printf("\nProceso %d - Matriz local_A:\n", rank);
    imprimir_matriz(local_A, N, local_col_count, "local_A", rank);
    printf("\nProceso %d - Matriz local_B:\n", rank);
    imprimir_matriz(local_B, N, local_col_count, "local_B", rank);

    // Calcular local_C = 2 * local_A + local_B
    for (int col = 0; col < local_col_count; col++) {
        for (int row = 0; row < N; row++) {
            local_C[row * local_col_count + col] =
                lambda * local_A[row * local_col_count + col] + local_B[row * local_col_count + col];
        }
    }

    printf("\nProceso %d - Matriz local_C:\n", rank);
    imprimir_matriz(local_C, N, local_col_count, "local_C", rank);

    // Recolectar resultados en C
    MPI_Gatherv(local_C, sendcounts[rank], MPI_DOUBLE, C, sendcounts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("\nMatriz C resultado final:\n");
        imprimir_matriz(C, N, N, "C", rank);
        guardar_matriz(C, N, "resultado.txt", rank, tam);
    }

    free(local_A);
    free(local_B);
    free(local_C);
    if (rank == 0) {
        free(A);
        free(B);
        free(C);
    }
    free(sendcounts);
    free(displs);

    MPI_Finalize();
    return 0;
}

void guardar_matriz(double* matrix, int N, const char* filename, int rank, int tam) {
    FILE* file = fopen(filename, "a");  // Modo append
    if (file == NULL) {
        fprintf(stderr, "Error al abrir el archivo %s\n", filename);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // Escribir un encabezado con información del proceso y tamaño
    fprintf(file, "==== Resultado para %d ====\n", tam);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fprintf(file, "%lf ", matrix[i * N + j]);
        }
        fprintf(file, "\n");
    }
    fprintf(file, "\n");  // Línea adicional para separación
    fclose(file);
}


void imprimir_matriz(double* matrix, int filas, int columnas, const char* nombre, int rank) {
    printf("Proceso %d - Matriz %s:\n", rank, nombre);
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            printf("%lf ", matrix[i * columnas + j]);
        }
        printf("\n");
    }
}
