#!/bin/bash
# ----------------------------------------
# MPI Job Script for CESGA
# ----------------------------------------

#SBATCH -J calculo_matrices          # Nombre del trabajo
#SBATCH -o salida_matrices_%j.o      # Archivo de salida estándar
#SBATCH -e errores_matrices_%j.e     # Archivo de errores
#SBATCH -N 2                         # Número de nodos solicitados
#SBATCH -n 8                         # Número total de tareas MPI
#SBATCH --ntasks-per-node=4          # Tareas por nodo
#SBATCH --time=00:10:00              # Tiempo máximo de ejecución (hh:mm:ss)
#SBATCH --mem=2G                     # Memoria por núcleo

# Cargar módulos necesarios
module load gcc openmpi/4.1.4_ft3    # Cargar OpenMPI y GCC

# Nombre del archivo fuente y del ejecutable
SOURCE_FILE="matrix.c"               # Archivo fuente del programa
EXECUTABLE="matrix"                  # Nombre del ejecutable

# Parámetros del programa
N=4                                  # Tamaño de la matriz
LAMBDA=2                           # Valor de lambda

# Compilar el programa con mpicc
mpicc -o $EXECUTABLE $SOURCE_FILE -lm

# Verificar si la compilación fue exitosa
if [ $? -ne 0 ]; then
    echo "Error al compilar el programa."
    exit 1
fi

# Ejecutar el programa con diferentes números de procesos (de 1 a 8)
for PROC in $(seq 1 8); do
    echo "Ejecutando con $PROC proceso(s) - N=$N, lambda=$LAMBDA:"
    mpirun -np $PROC ./$EXECUTABLE $N $LAMBDA
    echo "----------------------------------------"
done

echo "Ejecución completada"
