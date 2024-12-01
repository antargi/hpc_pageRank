# Nombre del ejecutable
EXEC = pagerank

# Archivos fuente
SRC = main.cpp Node.cpp Connection.cpp

# Directorios de inclusión y bibliotecas
INCLUDE = -I/home/wade/bsponmpi/include
LIBDIR = -L/home/wade/bsponmpi/lib

# Bibliotecas necesarias
LIBS = -lbsponmpi

# Compilador y ejecutor
MPICC = mpiCC
MPIRUN = mpirun

# Flags de compilación
CFLAGS = $(INCLUDE)
LDFLAGS = $(LIBDIR) $(LIBS)

# Regla por defecto: compilar
all: $(EXEC)

# Compilación del ejecutable
$(EXEC): $(SRC)
	$(MPICC) $(CFLAGS) -o $(EXEC) $(SRC) $(LDFLAGS)

# Ejecución del programa
run: $(EXEC)
	@if [ -z "$(NP)" ] || [ -z "$(ARGS)" ]; then \
		echo "Por favor, especifique el número de procesos y los argumentos. Ejemplo: make run NP=N_procesos ARGS=\"conexiones_N.txt relevancia_inicial\""; \
		exit 1; \
	fi
	$(MPIRUN) -np $(NP) ./$(EXEC) $(ARGS)

# Limpieza de archivos generados
clean:
	rm -f $(EXEC)

.PHONY: all run clean
