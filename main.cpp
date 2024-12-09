#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <cmath>
#include "Node.h"
#include "Connection.h"
#include <algorithm>
#include <unordered_map>

#define restrict __restrict__
extern "C" {
    #include "bsp.h"
}

int g_argc;
char** g_argv;

int totalNodos = 0;
bool globalConvergencia = false;
bool convergenciaLocal;

void bsp_main();

int main(int argc, char** argv) {
    void bsp_main();
    
    g_argc = argc;
    g_argv = argv;
    
    bsp_init(bsp_main, argc, argv);
    bsp_main();

    return 0;
}

void cargarDatos(const std::string& filename, double relevanciaInicial, 
    
    std::vector<Node>& nodos, std::vector<Connection>& conexiones) {
    size_t start = filename.find('_') + 1;
    size_t end = filename.find('.');
    std::string numeroStr = filename.substr(start, end - start);
    int maxNodeId = std::stoi(numeroStr) - 1;
    for (int i = 0; i <= maxNodeId; ++i) {
        nodos.emplace_back(i, relevanciaInicial);
    }
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << filename << std::endl;
        return;
    }
    std::string line;
    int connectionId = 0;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        int origin, destination;
        double weight;
        char comma;
        if (!(iss >> origin >> comma >> destination >> comma >> weight)) {
            std::cerr << "Error: Formato inválido en la línea: " << line << std::endl;
            continue;
        }
        conexiones.emplace_back(connectionId++, origin, destination, weight);
    }
    file.close();

    // std::cout << "Archivo cargado: " << filename << std::endl;
    // std::cout << "Nodos: " << nodos.size() << ", Conexiones: " << conexiones.size() << std::endl;
}

void enviarNodos(int num_procs,
                 const std::vector<Node>& nodos,
                 std::vector<Node>& localNodos) {
    
    int pid = bsp_pid();

    for (int dest = 0; dest < num_procs; ++dest) {
        // std::cout << "Proceso " << pid << " preparando nodos para el proceso " << dest << std::endl;

        for (const auto& nodo : nodos) {
            if (nodo.id % num_procs == dest) {
                if (dest == 0) {
                    localNodos.push_back(nodo);
                    // std::cout << "Nodo almacenado localmente: ID = " << nodo.id << ", Relevancia Actual = " << nodo.relevanciaActual << ", Relevancia Prev = " << nodo.relevanciaPrev << std::endl;
                } else {
                    int tagNodos = 0; 
                    bsp_send(dest, &tagNodos, &nodo, sizeof(Node));
                    // std::cout << "Nodo enviado al proceso " << dest << ": ID = " << nodo.id << ", Relevancia Actual = " << nodo.relevanciaActual << ", Relevancia Prev = " << nodo.relevanciaPrev << std::endl;
                }
            }
        }
    }
   
}


void enviarConexiones(int num_procs,
                      const std::vector<Connection>& conexiones,
                      std::vector<Connection>& localConexiones) {

    int pid = bsp_pid();

    for (int dest = 0; dest < num_procs; ++dest) {
        for (const auto& conexion : conexiones) {
            // Seleccionar conexiones según el ORIGEN
            if (conexion.origin % num_procs == dest) {
                if (dest == pid) {
                    localConexiones.push_back(conexion);
                    // std::cout << "Conexión almacenada localmente: ID = " << conexion.id << ", Origen = " << conexion.origin << ", Destino = " << conexion.destination << ", Peso = " << conexion.weight << std::endl;
                } else  {
                    int tagConexiones = 0; 
                    bsp_send(dest, &tagConexiones, &conexion, sizeof(Connection));
                    // std::cout << "Conexión enviada al proceso " << dest << ": ID = " << conexion.id << ", Origen = " << conexion.origin << ", Destino = " << conexion.destination << ", Peso = " << conexion.weight << std::endl;
                }
            }
        }
    }
}


void recibirNodos(std::vector<Node>& localNodos) {

    int numMensajes, totalTamaño, currentTag;
    int status;
    bsp_qsize(&numMensajes, &totalTamaño);

    // std::cout << "Proceso " << bsp_pid() << " recibirá " << numMensajes << " mensajes de nodos." << std::endl;

    for (int i = 0; i < numMensajes; ++i) {

        bsp_get_tag(&status, &currentTag);

        Node nodo;
        bsp_move(&nodo, status);
        localNodos.push_back(nodo);
        // std::cout << "Proceso " << bsp_pid() << " recibió Nodo: ID = " << nodo.id << ", Relevancia Actual = " << nodo.relevanciaActual << ", Relevancia Prev = " << nodo.relevanciaPrev << std::endl;
    }
}



void recibirConexiones(std::vector<Connection>& localConexiones) {

    int numMensajes, totalTamaño, currentTag;
    int status;    
    bsp_qsize(&numMensajes, &totalTamaño);

    // std::cout << "Proceso " << bsp_pid() << " recibirá " << numMensajes << " mensajes de conexiones." << std::endl;

    for (int i = 0; i < numMensajes; ++i) {
        bsp_get_tag(&status, &currentTag);
        Connection conexion;
        bsp_move(&conexion, status);
        localConexiones.push_back(conexion);
        // std::cout << "Proceso " << bsp_pid() << " recibió Conexión: ID = " << conexion.id << ", Origen = " << conexion.origin << ", Destino = " << conexion.destination << ", Peso = " << conexion.weight << std::endl;
    }
}


void enviarRelevanciasDirectas(const std::vector<Node>& localNodos,
                               const std::vector<Connection>& localConexiones) {
    int pid = bsp_pid();

    for (const auto& conexion : localConexiones) {
        // Encontrar el nodo origen
        auto itOrigen = std::find_if(localNodos.begin(), localNodos.end(),
                                     [&conexion](const Node& nodo) {
                                         return nodo.id == conexion.origin;
                                     });

        if (itOrigen != localNodos.end()) {
            double relevanciaOrigen = itOrigen->relevanciaActual;
            int procesoDestino = conexion.destination % bsp_nprocs();

            // Enviar relevancia al proceso correspondiente con el destino en el tag
            if (procesoDestino != pid) {
                bsp_send(procesoDestino, &conexion.destination, &relevanciaOrigen, sizeof(double));
                // std::cout << "Proceso " << pid << " envió relevancia " << relevanciaOrigen << " del nodo " << conexion.origin << " hacia el nodo destino " << conexion.destination << " en el proceso " << procesoDestino << std::endl;
            }
        }
    }
}


void calcularYEnviarRelevancia(std::vector<Node>& localNodos,
                               const std::vector<Connection>& localConexiones,
                               std::unordered_map<int, double>& relevanciasExternas,
                               double dampingFactor,
                               int totalNodos) {
    int pid = bsp_pid();
    int num_procs = bsp_nprocs();

    // Reiniciar la relevancia actual de todos los nodos locales
    for (auto& nodo : localNodos) {
        nodo.relevanciaPrev = nodo.relevanciaActual;
        nodo.relevanciaActual = (1.0 - dampingFactor) / totalNodos; // Teletransportación inicial
        // std::cout << "Proceso " << pid << " | Nodo " << nodo.id << " | Inicial R_actual: " << nodo.relevanciaActual << std::endl;
    }

    // Incorporar las relevancias externas recibidas de otros procesos
    for (auto& nodo : localNodos) {
        if (relevanciasExternas.count(nodo.id) > 0) {
            nodo.relevanciaActual += dampingFactor * relevanciasExternas[nodo.id];
            // std::cout << "Proceso " << pid << " | Nodo " << nodo.id << " | Relevancia externa añadida: " << relevanciasExternas[nodo.id] << " | R_actual después: " << nodo.relevanciaActual << std::endl;
        }
    }

    // Calcular contribuciones locales y enviar relevancias externas si es necesario
    for (const auto& nodo : localNodos) {
        int origen = nodo.id;
        double relevanciaOrigen = nodo.relevanciaPrev;

        // Obtener las conexiones salientes del nodo origen
        std::vector<Connection> conexionesSalientes;
        for (const auto& conexion : localConexiones) {
            if (conexion.origin == origen) {
                conexionesSalientes.push_back(conexion);
            }
        }

        int gradoSaliente = conexionesSalientes.size();

        if (gradoSaliente > 0) {
            double contribucion = relevanciaOrigen / gradoSaliente;

            for (const auto& conexion : conexionesSalientes) {
                int destino = conexion.destination;
                double contribucionPonderada = contribucion * conexion.weight;

                // Buscar el nodo destino localmente
                auto itDestino = std::find_if(localNodos.begin(), localNodos.end(),
                                              [destino](const Node& nodo) { return nodo.id == destino; });

                if (itDestino != localNodos.end()) {
                    // Actualizar relevancia si el nodo destino está local
                    itDestino->relevanciaActual += dampingFactor * contribucionPonderada;
                    std::cout << "Proceso " << pid << " | Nodo origen " << origen << " | Nodo destino local " << destino << " | Relevancia actual después de actualizar: " << itDestino->relevanciaActual << " | Contribución añadida: " << contribucionPonderada << std::endl;
                } else {
                    // Si el nodo destino no está local, enviar al proceso correspondiente
                    int procesoDestino = destino % num_procs;
                    bsp_send(procesoDestino, &destino, &contribucionPonderada, sizeof(double));
                    // std::cout << "Proceso " << pid << " | Nodo origen " << origen << " | Nodo destino remoto " << destino << " enviado al proceso " << procesoDestino << " | Contribución enviada: " << contribucionPonderada << std::endl;
                }
            }
        }
    }

    // Limpiar relevancias externas después de procesarlas
    relevanciasExternas.clear();
}

std::unordered_map<int, double> recibirRelevanciasDirectas() {
    std::unordered_map<int, double> relevanciasExternas;
    int numMensajes, totalTamaño;
    int status;
    bsp_qsize(&numMensajes, &totalTamaño);

    for (int i = 0; i < numMensajes; ++i) {
        int tagDestino;
        double relevanciaRecibida;

        bsp_get_tag(&status, &tagDestino);  // Leer el tag para obtener el nodo destino
        bsp_move(&relevanciaRecibida, sizeof(double));  // Mover relevancia

        // std::cout << "Proceso " << bsp_pid() << " | Relevancia recibida: " << relevanciaRecibida << " | Nodo destino: " << tagDestino << std::endl;

        relevanciasExternas[tagDestino] += relevanciaRecibida;

        // Imprimir los detalles de la acumulación
        // std::cout << "Proceso " << bsp_pid() << " | Nodo destino: " << tagDestino << " | Relevancia recibida: " << relevanciaRecibida << " | Acumulado en relevanciasExternas: " << relevanciasExternas[tagDestino] << std::endl;
    }

    return relevanciasExternas;
}


void bsp_main() {

    bsp_begin(bsp_nprocs());     

    int pid = bsp_pid();          
    int num_procs = bsp_nprocs(); 

    int tagSize = sizeof(int);
    bsp_set_tagsize(&tagSize);

    std::cout << "El valor del pid es: " << pid << std::endl;

    std::string filename = g_argc > 1 ? g_argv[1] : "conexiones.txt";
    double relevanciaInicial = (g_argc == 3) ? std::stod(g_argv[2]) : 1.0;

    std::vector<Node> nodos;
    std::vector<Connection> conexiones;

    std::vector<Node> localNodos;
    std::vector<Connection> localConexiones;

    bsp_push_reg(&totalNodos, sizeof(int));
    bsp_sync();

    bsp_push_reg(&globalConvergencia, sizeof(bool));
    bsp_sync();

    if (pid == 0) {
        cargarDatos(filename, relevanciaInicial, nodos, conexiones);
        enviarNodos(num_procs, nodos, localNodos);
        totalNodos = (int)nodos.size();
        // std::cout << "Proceso " << pid << " calculó totalNodos: " << totalNodos << std::endl;
        for (int dest = 1; dest < num_procs; ++dest) {
            bsp_put(dest, &totalNodos, &totalNodos, 0, sizeof(int));
        }
        
    }
    
    // std::cout << "Pid: " << pid << " Distribución realizada" << std::endl;

    if (pid != 0) {
        bsp_get(0, &totalNodos, 0, &totalNodos, sizeof(int));
        // std::cout << "Proceso " << pid << " recibió totalNodos: " << totalNodos << std::endl;
    }


    bsp_sync();


    if (pid != 0) {
        // std::cout << "Pid: " << pid << " entrando al recibimiento de nodos" << std::endl;
        recibirNodos(localNodos);
    }        

    if (pid == 0) {        
        enviarConexiones(num_procs, conexiones, localConexiones);
    } 

    // std::cout << "Pid: " << pid << " esperando envío de conexiones..." << std::endl;
    bsp_sync();

    if( pid != 0 ){
        recibirConexiones(localConexiones);  
    }

    bsp_sync();      


    constexpr double epsilon = 1e-6;  // Umbral para la convergencia
    constexpr double dampingFactor = 0.85;  // Factor de amortiguamiento
    int iteracion = 0;
    constexpr int maxIteraciones = 60;
    std::unordered_map<int, double> relevanciasExternas; // Definir fuera del bucle

    bsp_push_reg(&convergenciaLocal, sizeof(bool));
    bsp_sync();

    while (!globalConvergencia && iteracion < maxIteraciones) {

        iteracion++;

        std::cout << "Iteración | " << iteracion << std::endl;


        // Paso 1: Calcular relevancias parciales locales
        // std::cout << "Proceso " << pid << " | Antes de calcular relevancias:" << std::endl;
        for (const auto& nodo : localNodos) {
            // std::cout << "Nodo " << nodo.id << " | R_prev: " << nodo.relevanciaPrev << " | R_actual: " << nodo.relevanciaActual << std::endl;
        }

        // calcularRelevanciaActual(localNodos, localConexiones, relevanciasExternas, dampingFactor, totalNodos);
        calcularYEnviarRelevancia(localNodos, localConexiones, relevanciasExternas, dampingFactor, totalNodos);
        
        // std::cout << "Proceso " << pid << " | Después de calcular relevancias parciales:" << std::endl;
        for (const auto& nodo : localNodos) {
            // std::cout << "Nodo " << nodo.id << " | R_actual: " << nodo.relevanciaActual << std::endl;
        }

        bsp_sync(); 

        // Debug: Imprimir relevancias locales calculadas
        // std::cout << "Iteración " << iteracion << " - Relevancias parciales locales:" << std::endl;
        for (const auto& nodo : localNodos) {
            // std::cout << "Nodo " << nodo.id << " | Relevancia Parcial: " << nodo.relevanciaActual << " | Relevancia Previa: " << nodo.relevanciaPrev << std::endl;
        }

        // Paso 2: Enviar relevancias directamente a procesos destino
        // enviarRelevanciasDirectas(localNodos, localConexiones); 

        // bsp_sync();

        // Paso 3: Recibir relevancias externas
        auto nuevasRelevancias = recibirRelevanciasDirectas();
        relevanciasExternas = std::move(nuevasRelevancias);

        bsp_sync();

        convergenciaLocal = true; 
        for (const auto& nodo : localNodos) {
            if (std::fabs(nodo.relevanciaActual - nodo.relevanciaPrev) > epsilon) {
                std::cout << "Proceso: " << pid << " Nodo: " << nodo.id << " Convergencia: " << fabs(nodo.relevanciaActual - nodo.relevanciaPrev)  <<  std::endl;
                convergenciaLocal = false;
                break; 
            }
        }


        // std::cout << "Proceso: " << pid << " Convergencia local: " << convergenciaLocal <<  std::endl;
        if (pid != 0) {
            bsp_put(0, &convergenciaLocal, &convergenciaLocal, pid * sizeof(bool), sizeof(bool));
        }

        bsp_sync();

        if (pid == 0) {
            globalConvergencia = true; 
            for (int p = 1; p < num_procs; ++p) {
                bsp_get(p, &convergenciaLocal, p * sizeof(bool), &convergenciaLocal, sizeof(bool));
                // std::cout << "Proceso 0 | Convergencia recibida de proceso " << p << ": " << convergenciaLocal << std::endl;
                if (!convergenciaLocal) {
                    globalConvergencia = false;
                }
            }
            // std::cout << "Proceso: " << pid << " Convergencia Global: " << globalConvergencia <<  std::endl;
            for (int p = 1; p < num_procs; ++p) {
                bsp_put(p, &globalConvergencia, &globalConvergencia, 0, sizeof(bool));
            }
        }

        bsp_sync();

        std::cout << "Proceso " << pid << " ha convergido." << std::endl;
        for (const auto& nodo : localNodos) {
            std::cout << "Proceso: " << pid << " Nodo: " << nodo.id
                    << " Relevancia final: " << nodo.relevanciaActual << std::endl;
        }
        if (globalConvergencia) {
        }
    }


    bsp_end();
}
