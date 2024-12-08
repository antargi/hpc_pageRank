#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <cmath>
#include "Node.h"
#include "Connection.h"
#include <unordered_map>

#define restrict __restrict__
extern "C" {
    #include "bsp.h"
}

int g_argc;
char** g_argv;


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

    std::cout << "Archivo cargado: " << filename << std::endl;
    std::cout << "Nodos: " << nodos.size() << ", Conexiones: " << conexiones.size() << std::endl;
}

void enviarNodos(int num_procs,
                 const std::vector<Node>& nodos,
                 std::vector<Node>& localNodos) {
    
    int pid = bsp_pid();

    for (int dest = 0; dest < num_procs; ++dest) {
        std::cout << "Proceso " << pid << " preparando nodos para el proceso " << dest << std::endl;

        for (const auto& nodo : nodos) {
            if (nodo.id % num_procs == dest) {
                if (dest == 0) {
                    localNodos.push_back(nodo);
                    std::cout << "Nodo almacenado localmente: ID = " << nodo.id
                              << ", Relevancia Actual = " << nodo.relevanciaActual
                              << ", Relevancia Prev = " << nodo.relevanciaPrev << std::endl;
                } else {
                    int tagNodos = 0; 
                    bsp_send(dest, &tagNodos, &nodo, sizeof(Node));
                    std::cout << "Nodo enviado al proceso " << dest << ": ID = " << nodo.id
                              << ", Relevancia Actual = " << nodo.relevanciaActual
                              << ", Relevancia Prev = " << nodo.relevanciaPrev << std::endl;
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
                    std::cout << "Conexión almacenada localmente: ID = " << conexion.id
                              << ", Origen = " << conexion.origin
                              << ", Destino = " << conexion.destination
                              << ", Peso = " << conexion.weight << std::endl;
                } else  {
                    int tagConexiones = 0; 
                    bsp_send(dest, &tagConexiones, &conexion, sizeof(Connection));
                    std::cout << "Conexión enviada al proceso " << dest << ": ID = " << conexion.id
                              << ", Origen = " << conexion.origin
                              << ", Destino = " << conexion.destination
                              << ", Peso = " << conexion.weight << std::endl;
                }
            }
        }
    }
}


void recibirNodos(std::vector<Node>& localNodos) {

    int numMensajes, totalTamaño, currentTag;
    int status;
    bsp_qsize(&numMensajes, &totalTamaño);

    std::cout << "Proceso " << bsp_pid() << " recibirá " << numMensajes << " mensajes de nodos." << std::endl;

    for (int i = 0; i < numMensajes; ++i) {

        bsp_get_tag(&status, &currentTag);

        Node nodo;
        bsp_move(&nodo, status);
        localNodos.push_back(nodo);
        std::cout << "Proceso " << bsp_pid() << " recibió Nodo: ID = " << nodo.id
                    << ", Relevancia Actual = " << nodo.relevanciaActual
                    << ", Relevancia Prev = " << nodo.relevanciaPrev << std::endl;
    }
}



void recibirConexiones(std::vector<Connection>& localConexiones) {

    int numMensajes, totalTamaño, currentTag;
    int status;    
    bsp_qsize(&numMensajes, &totalTamaño);

    std::cout << "Proceso " << bsp_pid() << " recibirá " << numMensajes << " mensajes de conexiones." << std::endl;

    for (int i = 0; i < numMensajes; ++i) {
        bsp_get_tag(&status, &currentTag);
        Connection conexion;
        bsp_move(&conexion, status);
        localConexiones.push_back(conexion);
        std::cout << "Proceso " << bsp_pid() << " recibió Conexión: ID = " << conexion.id
                    << ", Origen = " << conexion.origin
                    << ", Destino = " << conexion.destination
                    << ", Peso = " << conexion.weight << std::endl;
    }
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
    
    if (pid == 0) {
        cargarDatos(filename, relevanciaInicial, nodos, conexiones);
        enviarNodos(num_procs, nodos, localNodos);
    } 

    std::cout << "Pid: " << pid << " esperando envío de nodos..." << std::endl;
    bsp_sync();
    
    if (pid != 0) {
        std::cout << "Pid: " << pid << " entrando al recibimiento de nodos" << std::endl;
        recibirNodos(localNodos);
    }        


    if (pid == 0) {        
        enviarConexiones(num_procs, conexiones, localConexiones);
    } 

    std::cout << "Pid: " << pid << " esperando envío de conexiones..." << std::endl;
    bsp_sync();

    if( pid != 0 ){
        recibirConexiones(localConexiones);        
    }


    bsp_end();

    return;

    // bsp_sync();
    // recibirConexiones(localConexiones, tagConexiones);
   

    // bsp_sync();

    constexpr double epsilon = 1e-6;
    bool convergencia = false;
    int iteracion = 0;

    while (!convergencia) {
        iteracion++;

        std::vector<int> nodosRequeridos;
        solicitarNodosExternos(localConexiones, nodosRequeridos);

        enviarRelevancias(localNodos, nodosRequeridos);
        bsp_sync();
        auto relevanciasExternas = recibirRelevancias();

        actualizarRelevancias(localNodos, localConexiones, relevanciasExternas);

        convergencia = verificarConvergencia(localNodos, epsilon, num_procs);

        if (pid == 0) {
            std::cout << "Iteración " << iteracion << " - Convergencia: " << (convergencia ? "Sí" : "No") << std::endl;
        }

        bsp_sync();
    }

    if (pid == 0) {
        std::cout << "Convergencia alcanzada tras " << iteracion << " iteraciones." << std::endl;
    }

    //  while (!convergencia) {
    //      iteracion++;

    //      std::vector<int> nodosRequeridos;

    // }

    bsp_end();

    return;
}
