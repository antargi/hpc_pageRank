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
constexpr double epsilon = 1e-2;  // Umbral para la convergencia
constexpr double dampingFactor = 0.85;  // Factor de amortiguamiento

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
    relevanciaInicial = 1.0 / (maxNodeId + 1);
    for (int i = 0; i <= maxNodeId; ++i) {
        nodos.emplace_back(i, relevanciaInicial, 0, 0, 0);
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

    //For each node add the number of incoming and outgoing connections
    for (auto& node : nodos) {

        node.relevanciaPrev = (1-dampingFactor)/nodos.size();
        for (const auto& connection : conexiones) {
            if (connection.origin == node.id) {
                node.outCon++;
            }
            if (connection.destination == node.id) {
                node.inCon++;
            }
        }
    }

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
            if (conexion.destination % num_procs == dest) {
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



// Función corregida de calcularRelevancia
void calcularRelevancia(std::vector<Node>& localNodos,
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
    }

    // Deteterminar cantidad de conexiones entrantes de cada nodo
    int conexionesEntrantesNodo;
    double newRelevancia;
    // Calcular contribuciones locales y recibir relevancias externas si es necesario
    for (auto& nodo : localNodos) {
        newRelevancia = 0;
        if (relevanciasExternas.count(nodo.id) > 0) {
            //std::cout << "Proceso " << pid << " | Nodo " << nodo.id << " | Relevancia Externa: " << relevanciasExternas[nodo.id] << std::endl;
            newRelevancia += dampingFactor * relevanciasExternas[nodo.id];
        }

        int destination = nodo.id;

        // Obtener las conexiones entrantes del nodo origen
        std::vector<Connection> conexionesEntrantes;
        for (const auto& conexion : localConexiones) {
            if (conexion.destination == destination) {
                conexionesEntrantes.push_back(conexion);
            }
        }

        for (const auto& conexion : conexionesEntrantes) {
            int origen = conexion.origin;
            // Buscar el nodo origen localmente
            auto itOrigen = std::find_if(localNodos.begin(), localNodos.end(),
                                          [origen](const Node& nodo) { return nodo.id == origen; });

            if (itOrigen != localNodos.end()) {
                double relevanciaOrigen = itOrigen->relevanciaPrev;
                double contribucion = relevanciaOrigen * conexion.weight * dampingFactor/itOrigen->outCon;
                // Imprimir las contribuciones antes de actualizar la relevancia
                // Imprimir las contribuciones locales
                if (nodo.id == 2){
                    //std::cout << "Nodo " << nodo.id << " | Contribución Local desde Nodo " << origen 
                      //      << ": " << relevanciaOrigen << " | Contribución Ponderada: " << contribucion << std::endl;
                }
                newRelevancia += contribucion;
            }
        }
        nodo.relevanciaActual += newRelevancia;
        
    }
    // Limpiar relevancias externas después de procesarlas
    relevanciasExternas.clear();
}

// Función corregida de solicitarRelevancias
void solicitarRelevancias(std::vector<Node>& localNodos, const std::vector<Connection>& localConexiones) {
    int pid = bsp_pid();
    int num_procs = bsp_nprocs();

    for (const auto& nodo : localNodos) {
        //std::cout << "Proceso en solicitar " << pid << " | Nodo " << nodo.id << " | Relevancia Actual: " << nodo.relevanciaActual << std::endl;
        int destination = nodo.id;

        // Obtener las conexiones entrantes del nodo origen
        std::vector<Connection> conexionesEntrantes;
        for (const auto& conexion : localConexiones) {
            //std::cout << "Conexión: " << conexion.origin << " -> " << conexion.destination << std::endl;
            if (conexion.destination == destination) {
                conexionesEntrantes.push_back(conexion);
            }
        }
        //std::cout << "Proceso en solicitar conex" << pid << " | Nodo " << nodo.id << " | Conexiones entrantes: " << conexionesEntrantes.size() << std::endl;

        for (const auto& conexion : conexionesEntrantes) {
            int origen = conexion.origin;
            // Buscar el nodo origen localmente
            auto itOrigen = std::find_if(localNodos.begin(), localNodos.end(),
                                          [origen](const Node& nodo) { return nodo.id == origen; });

            if (itOrigen == localNodos.end()) { // Nodo origen no está local
                int procesoDestino = origen % num_procs;
                //std::cout << "el proceso" << pid << "solicta la relevancia del nodo" << origen << "al proceso" << procesoDestino << std::endl;
                bsp_send(procesoDestino, &destination, &origen, sizeof(int));
            }
        }
    }
}

void responderRelevancia(int pid, int num_procs, std::vector<Node>& localNodos) {
    int numMensajes, totalTamaño, destinoTag;
    int status;
    
    // Esperar a recibir las solicitudes de relevancia
    bsp_qsize(&numMensajes, &totalTamaño);
  //std:: << "Proceso " << pid << " recibirá " << numMensajes << " solicitudes de relevancia." << std::endl;
    for (int i = 0; i < numMensajes; ++i) {
        //std::cout << "Proceso " << pid << " recibió solicitud de relevancia." << std::endl;
        int nodoOrigen;
        int nodoDestino;

        // Recibir el tag que indica el nodo destino
        bsp_get_tag(&status, &destinoTag);
        bsp_move(&nodoOrigen, sizeof(int));
      //std:: << "Proceso " << pid << " recibió solicitud de relevancia del nodo " << nodoOrigen << " al nodo " << destinoTag << std::endl;
        //std::cout << "Proceso " << pid << " recibió solicitud de relevancia del nodo " << nodoOrigen << " al nodo " << nodoDestino << std::endl;

        // Buscar la relevancia del nodo origen
        for (auto& nodo : localNodos) {
            //std::cout << "Proceso " << pid << " | Nodo " << nodo.id << " | Relevancia Actual: " << nodo.relevanciaActual << "se esta buscando el nodo "<< nodoOrigen<< std::endl;
            if (nodo.id == nodoOrigen) {
                double relevanciaOrigen = nodo.relevanciaPrev; // Relevancia previa del nodo origen
                //std::cout << "Proceso " << pid << " | Nodo " << nodoOrigen << " | Relevancia Actual: " << nodo.relevanciaActual << " | Relevancia Prev: " << nodo.relevanciaPrev << std::endl;
                // Enviar la relevancia al proceso solicitante
                if (nodo.outCon > 0) {
                    //std::cout << "Se dividio la relevancia del nodo " << nodoOrigen << " entre " << nodo.outCon << std::endl;
                    relevanciaOrigen = relevanciaOrigen / nodo.outCon;
                }
                int procesoDestino = destinoTag % num_procs;
                bsp_send(procesoDestino, &destinoTag, &relevanciaOrigen, sizeof(double));
                if (destinoTag == 2){
                    //std::cout << "Se respondió la solicitud de relevancia del nodo " << nodoOrigen << " al nodo " << destinoTag << " con relevancia " << relevanciaOrigen << " En el procesador " << pid <<std::endl;
                }
        }
        }
        }

    }


std::unordered_map<int, double> recibirRelevanciasDirectas() {
    std::unordered_map<int, double> relevanciasExternas;
    int numMensajes, totalTamaño;
    int status;
    
    // Consultamos cuántos mensajes hay en la cola de BSP
    bsp_qsize(&numMensajes, &totalTamaño);

    for (int i = 0; i < numMensajes; ++i) {
        int tagDestino;
        double relevanciaRecibida;

        // Obtener el tag, que identifica el nodo destino al cual pertenece la relevancia
        bsp_get_tag(&status, &tagDestino);  
        
        // Mover la relevancia recibida a la variable relevanciaRecibida
        bsp_move(&relevanciaRecibida, sizeof(double));

        //std::cout << "Proceso " << bsp_pid() << " | Nodo destino: " << tagDestino 
          //        << " | Relevancia recibida: " << relevanciaRecibida << std::endl;
        // Acumulamos la relevancia recibida en el nodo destino correspondiente
        relevanciasExternas[tagDestino] += relevanciaRecibida;

        // (Opcional) Imprimir detalles para verificar el comportamiento
        //std::cout << "Proceso " << bsp_pid() << " | Nodo destinorecibir: " << tagDestino 
          //        << " | Relevancia recibida: " << relevanciaRecibida 
            //      << " | Acumulado: " << relevanciasExternas[tagDestino] << std::endl;
    }

    return relevanciasExternas;
}



void bsp_main() {

    bsp_begin(bsp_nprocs());     

    int pid = bsp_pid();          
    int num_procs = bsp_nprocs(); 

    int tagSize = sizeof(int);
    bsp_set_tagsize(&tagSize);

    //std::cout << "El valor del pid es: " << pid << std::endl;

    std::string filename = g_argc > 1 ? g_argv[1] : "conexiones.txt";

    std::vector<Node> nodos;
    std::vector<Connection> conexiones;

    std::vector<Node> localNodos;
    std::vector<Connection> localConexiones;

    bsp_push_reg(&totalNodos, sizeof(int));
    bsp_sync();

    double relevanciaInicial = (g_argc == 3) ? std::stod(g_argv[2]) : 1.0;

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

    //For each proccess print nodes and values

    for (int i = 0; i < num_procs; ++i) {
        if (pid == i) {
            std::cout << "Proceso " << pid << " | Nodos locales: " << localNodos.size() << std::endl;
            for (const auto& nodo : localNodos) {
                std::cout << "Proceso " << pid << " | Nodo: " << nodo.id << " | Relevancia Actual: " << nodo.relevanciaActual << " | Relevancia Prev: " << nodo.relevanciaPrev << " | outCon" << nodo.outCon << " | inCon" << nodo.inCon << std::endl;
            }
            std::cout << "Proceso " << pid << " | Conexiones locales: " << localConexiones.size() << std::endl;
            for (const auto& conexion : localConexiones) {
                std::cout << "Proceso " << pid << " | Conexión: " << conexion.id << " | Origen: " << conexion.origin << " | Destino: " << conexion.destination << " | Peso: " << conexion.weight << std::endl;
            }
        }
        bsp_sync();
    }


    
    int iteracion = 0;
    constexpr int maxIteraciones = 10;
    std::unordered_map<int, double> relevanciasExternas; 

    bsp_push_reg(&convergenciaLocal, sizeof(bool));
    bsp_sync();

    while (!globalConvergencia) {

        iteracion++;

        
        //std::cout << "Iteración | " << iteracion << std::endl;


        //std::cout << "Proceso " << pid << " | Antes de calcular relevancias:" << std::endl;
        for (const auto& nodo : localNodos) {
            //std::cout << "Nodo " << nodo.id << " | R_prev: " << nodo.relevanciaPrev << " | R_actual: " << nodo.relevanciaActual << std::endl;
        }
        // std::cout << "Proceso " << pid << " | Después de calcular relevancias parciales:" << std::endl;
        for (const auto& nodo : localNodos) {
            // std::cout << "Nodo " << nodo.id << " | R_actual: " << nodo.relevanciaActual << std::endl;
        }


        // Debug: Imprimir relevancias locales calculadas
        //std::cout << "Iteración " << iteracion << " - Relevancias parciales locales:" << std::endl;
        for (const auto& nodo : localNodos) {
            //std::cout << "Nodo " << nodo.id << " | Relevancia Parcial: " << nodo.relevanciaActual << " | Relevancia Previa: " << nodo.relevanciaPrev << std::endl;
        }


        // Paso 1: Solicitar relevancias directamente a procesos origenes
        solicitarRelevancias(localNodos, localConexiones);

        bsp_sync();

        // Paso 2: Responder a solicitudes de relevancia
        responderRelevancia(pid, num_procs, localNodos);

        bsp_sync();

        // Paso 3: Recibir relevancias externas
        auto nuevasRelevancias = recibirRelevanciasDirectas();

        relevanciasExternas = std::move(nuevasRelevancias);

        //std:: << "Proceso " << pid << " | Relevancias externas recibidas:" << relevanciasExternas.size() << std::endl;
        for (const auto& par : relevanciasExternas) {
            //std::cout << "Proceso " << pid << " | Nodo: " << par.first << " | Relevancia: " << par.second << std::endl;
        }

        bsp_sync();

        // Paso 4: Calcular relevancias localess
        calcularRelevancia(localNodos, localConexiones, relevanciasExternas, dampingFactor, totalNodos);
        
        bsp_sync();
    

        convergenciaLocal = true; 
        for (const auto& nodo : localNodos) {
            if (std::fabs(nodo.relevanciaActual - nodo.relevanciaPrev) > epsilon) {
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
            for (int p = 0; p < num_procs; ++p) {
                bsp_get(p, &convergenciaLocal, p * sizeof(bool), &convergenciaLocal, sizeof(bool));
                //std::cout << "Proceso" << p << " | Convergencia recibida: " << convergenciaLocal << std::endl;
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

        if (convergenciaLocal) {
            std::cout << "Proceso " << pid << " ha convergido." << std::endl;
        }
        if (globalConvergencia) {
            for (const auto& nodo : localNodos) {
            std::cout << "Proceso: " << pid << " Nodo: " << nodo.id
                    << " Relevancia final: " << nodo.relevanciaActual << std::endl;
        }
        }
    }


    bsp_end();
}
