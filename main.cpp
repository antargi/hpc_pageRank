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

void distribuirDatos(const std::vector<Node>& nodos,
                     const std::vector<Connection>& conexiones,
                     int num_procs, int pid,
                     std::vector<Node>& localNodos,
                     std::vector<Connection>& localConexiones) {
    // distribuir nodos
    for (const auto& nodo : nodos) {
        if (nodo.id % num_procs == pid) {
            localNodos.push_back(nodo);
        }
    }

    // distribuir conn
    for (const auto& conexion : conexiones) {
        if (conexion.destination % num_procs == pid) {
            localConexiones.push_back(conexion);
        }
    }
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
                 std::vector<Node>& localNodos,
                 int tagNodos) {

    int tagSize = sizeof(int);
    bsp_set_tagsize(&tagSize); 
    
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
                 std::vector<Connection>& localConexiones,
                 int tagConexiones) {

    int tagSize = sizeof(int);
    bsp_set_tagsize(&tagSize); 
    
    int pid = bsp_pid();

    for (int dest = 0; dest < num_procs; ++dest) {
        // std::cout << "Proceso " << pid << " preparando conexiones para el proceso " << dest << std::endl;

        for (const auto& conexion : conexiones) {
            if (conexion.destination % num_procs == dest) {
                if (dest == 0) {
                    localConexiones.push_back(conexion);
                    std::cout << "Conexión almacenada localmente: ID = " << conexion.id
                              << ", Origen = " << conexion.origin
                              << ", Destino = " << conexion.destination
                              << ", Peso = " << conexion.weight << std::endl;
                } else {
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


void recibirNodos(std::vector<Node>& localNodos, int tagNodos) {

    int numMensajes, totalTamaño, currentTag;
    bsp_qsize(&numMensajes, &totalTamaño);

    std::cout << "Proceso " << bsp_pid() << " recibirá " << numMensajes << " mensajes de nodos." << std::endl;

    for (int i = 0; i < numMensajes; ++i) {

        bsp_get_tag(&currentTag, nullptr);

        Node nodo;
        bsp_move(&nodo, sizeof(Node));
        localNodos.push_back(nodo);
        std::cout << "Proceso " << bsp_pid() << " recibió Nodo: ID = " << nodo.id
                    << ", Relevancia Actual = " << nodo.relevanciaActual
                    << ", Relevancia Prev = " << nodo.relevanciaPrev << std::endl;
    }
}



void recibirConexiones(std::vector<Connection>& localConexiones, int tagConexiones) {

    int numMensajes, totalTamaño, currentTag;
    bsp_qsize(&numMensajes, &totalTamaño);

    // std::cout << "Proceso " << bsp_pid() << " recibirá " << numMensajes << " mensajes de conexiones." << std::endl;

    for (int i = 0; i < numMensajes; ++i) {
        bsp_get_tag(&currentTag, nullptr);
        Connection conexion;
        bsp_move(&conexion, sizeof(Connection));
        localConexiones.push_back(conexion);
        std::cout << "Proceso " << bsp_pid() << " recibió Conexión: ID = " << conexion.id
                    << ", Origen = " << conexion.origin
                    << ", Destino = " << conexion.destination
                    << ", Peso = " << conexion.weight << std::endl;
    }
}







bool verificarConvergencia(const std::vector<Node>& localNodos, double epsilon, int num_procs) {
    bool localConvergencia = true;

    for (const auto& nodo : localNodos) {
        if (std::abs(nodo.relevanciaActual - nodo.relevanciaPrev) >= epsilon) {
            localConvergencia = false;
            break;
        }
    }

    bsp_push_reg(&localConvergencia, sizeof(bool));
    bsp_sync();

    bool globalConvergencia = localConvergencia;
    for (int i = 0; i < num_procs; ++i) {
        bool procesoConvergencia;
        bsp_get(i, &localConvergencia, 0, &procesoConvergencia, sizeof(bool));
        globalConvergencia = globalConvergencia && procesoConvergencia;
    }

    return globalConvergencia;
}


void enviarRelevancias(const std::vector<Node>& localNodos, const std::vector<int>& nodosRequeridos) {
    for (int destino = 0; destino < bsp_nprocs(); ++destino) {
        if (destino != bsp_pid()) {
            for (int nodoId : nodosRequeridos) {
                for (const auto& nodo : localNodos) {
                    if (nodo.id == nodoId) {
                        bsp_send(destino, nullptr, &nodo, sizeof(Node));
                        break;
                    }
                }
            }
        }
    }
}



void solicitarNodosExternos(const std::vector<Connection>& localConexiones, std::vector<int>& nodosRequeridos) {
    for (const auto& conexion : localConexiones) {
        if (conexion.origin % bsp_nprocs() != bsp_pid()) {
            nodosRequeridos.push_back(conexion.origin);
        }
    }
}


void actualizarRelevancias(std::vector<Node>& localNodos, const std::vector<Connection>& localConexiones, const std::unordered_map<int, double>& relevanciasExternas) {
    for (auto& nodo : localNodos) {
        nodo.relevanciaPrev = nodo.relevanciaActual;
        nodo.relevanciaActual = 0.0;
    }

    for (const auto& conexion : localConexiones) {
        for (auto& nodo : localNodos) {
            if (nodo.id == conexion.destination) {
                double relevanciaOrigen = 0.0;
                if (conexion.origin % bsp_nprocs() == bsp_pid()) {
                    relevanciaOrigen = localNodos[conexion.origin].relevanciaPrev;
                } else {
                    relevanciaOrigen = relevanciasExternas.at(conexion.origin);
                }
                nodo.relevanciaActual += relevanciaOrigen * conexion.weight;
            }
        }
    }
}



std::unordered_map<int, double> recibirRelevancias() {
    std::unordered_map<int, double> relevanciasExternas;
    int numMensajes, totalTamaño;
    bsp_qsize(&numMensajes, &totalTamaño);

    for (int i = 0; i < numMensajes; ++i) {
        Node nodo;
        bsp_move(&nodo, sizeof(Node));
        relevanciasExternas[nodo.id] = nodo.relevanciaPrev; 
    }

    return relevanciasExternas;
}



void bsp_main() {

    bsp_begin(bsp_nprocs());     

    int pid = bsp_pid();          
    int num_procs = bsp_nprocs(); 

    std::cout << "El valor del pid es: " << pid << std::endl;

    std::string filename = g_argc > 1 ? g_argv[1] : "conexiones.txt";
    double relevanciaInicial = (g_argc == 3) ? std::stod(g_argv[2]) : 1.0;

    std::vector<Node> nodos;
    std::vector<Connection> conexiones;

    std::vector<Node> localNodos;
    std::vector<Connection> localConexiones;
    
    int tagNodos = 10;
    int tagConexiones = 20;

    if (pid == 0) {
        cargarDatos(filename, relevanciaInicial, nodos, conexiones);
        enviarNodos(num_procs, nodos, localNodos, tagNodos);
    }         
    
    bsp_sync();
    recibirNodos(localNodos, tagNodos);


    if (pid == 0) {        
        enviarConexiones(num_procs, conexiones, localConexiones, tagConexiones);
    } 

    bsp_sync();
    recibirConexiones(localConexiones, tagConexiones);
   

    bsp_sync();

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

    bsp_end();

    // while (!convergencia) {
    //     iteracion++;

    //     actualizarRelevancias(localNodos, localConexiones, nodos);

    //     convergencia = verificarConvergencia(localNodos, epsilon, num_procs);

    //     if (pid == 0) {
    //         std::cout << "Iteración " << iteracion << ":" << std::endl;
    //         for (const auto& nodo : nodos) {
    //             std::cout << "Nodo " << nodo.id << " | Relevancia Actual: "
    //                       << nodo.relevanciaActual << std::endl;
    //         }
    //     }
        
    //     bsp_sync();
    // }

    // if (pid == 0) {
    //     std::cout << "\nConvergencia alcanzada en " << iteracion << " iteraciones." << std::endl;
    //     std::cout << "Resultados finales:" << std::endl;
    //     for (const auto& nodo : nodos) {
    //         std::cout << "Nodo " << nodo.id << " | Relevancia Final: " 
    //                   << nodo.relevanciaActual << std::endl;
    //     }
    // }

    // bsp_end();
}
