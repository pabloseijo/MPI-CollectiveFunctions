#!/bin/bash
#SBATCH -J escalabilidad_matrices     # Nombre del trabajo
#SBATCH -o salida_%j.o                # Archivo de salida estándar
#SBATCH -e errores_%j.e               # Archivo de errores
#SBATCH -N 8                          # Número de nodos solicitados
#SBATCH -n 32                         # Número total de tareas MPI
#SBATCH --ntasks-per-node=4           # Tareas por nodo
#SBATCH --time=02:00:00               # Tiempo máximo de ejecución (hh:mm:ss)
#SBATCH --mem=16G                     # Memoria total por nodo

# Cargar módulos necesarios
module load gcc openmpi/4.1.4_ft3

# Parámetros del programa
SOURCE_FILE="matrix.c"                # Archivo fuente del programa
EXECUTABLE="matrix"                   # Nombre del ejecutable
OUTPUT_FILE="resultados_grandes_intermedios.txt"  # Archivo donde se guardarán los resultados

# Compilar el programa con mpicc
mpicc -o $EXECUTABLE $SOURCE_FILE -lm
if [ $? -ne 0 ]; then
    echo "Error al compilar el programa."
    exit 1
fi

# Borrar archivo de resultados anterior
rm -f $OUTPUT_FILE
echo "Procesos,N,Iteración,Tiempo" >> $OUTPUT_FILE

# Parámetros específicos
LAMBDAS=(1000000.0)                   # Valor grande de lambda (1e6)
C_MAX=10                              # Tamaño máximo de columnas por proceso

# Tamaños grandes de la matriz con valores intermedios adicionales
N_SIZES=(500 750 1000 1250 1500 1750 2000 2500 3000 3500 4000 5000 6000 7000 8000 10000 12000 14000 16000 18000 20000 24000 28000 32000)

# Número creciente de procesos MPI
PROCESSES=(2 4 8 16 24 32 64)

# Ejecutar cada configuración SOLO una vez
for N in "${N_SIZES[@]}"; do
    for PROC in "${PROCESSES[@]}"; do
        echo "Ejecutando con $PROC procesos, N=$N y lambda=${LAMBDAS[0]}..."
        OUTPUT=$(mpirun -np $PROC ./$EXECUTABLE $N ${LAMBDAS[0]} $C_MAX)
        
        # Extraer tiempo de ejecución desde la salida del programa
        TIME=$(echo "$OUTPUT" | grep 'Tiempo total' | awk '{print $5}')
        
        # Guardar resultados en el archivo
        echo "$PROC,$N,1,$TIME" >> $OUTPUT_FILE
    done
done

echo "Ejecución completada. Resultados almacenados en $OUTPUT_FILE"
