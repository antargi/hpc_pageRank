#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <cmath>
#include "Node.h"
#include "Connection.h"
#define restrict __restrict__
extern "C" {
    #include "bsp.h" 
}

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Uso: " << argv[0] << " <nombre_del_archivo> [relevancia_inicial]\n";
        return 1;
    }

    std::string filename = argv[1];
    double relevanciaInicial = (argc == 3) ? std::stod(argv[2]) : 1.0;

    std::vector<Node> nodos;
    std::vector<Connection> conexiones;

    // Obtener el número de nodos del nombre del archivo
    size_t start = filename.find('_') + 1;
    size_t end = filename.find('.');
    std::string numeroStr = filename.substr(start, end - start);
    int maxNodeId = std::stoi(numeroStr) - 1;

    // Crear los nodos con la relevancia inicial
    for (int i = 0; i <= maxNodeId; ++i) {
        nodos.emplace_back(i, relevanciaInicial);
    }

    // Leer el archivo y construir las conexiones
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << filename << std::endl;
        return 1;
    }

    std::string line;
    int connectionId = 0;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        int origin, destination;
        double weight;
        char comma;

        // Leer los valores de la línea
        if (!(iss >> origin >> comma >> destination >> comma >> weight)) {
            std::cerr << "Error: Formato inválido en la línea: " << line << std::endl;
            continue;
        }

        // Crear una conexión y agregarla al vector
        conexiones.emplace_back(connectionId++, origin, destination, weight);
    }

    file.close();

    // Iteraciones para actualizar las relevancias hasta la convergencia
    constexpr double epsilon = 1e-6;
    bool convergencia = false;
    int iteracion = 0;

    std::cout << "Iteración " << 0 << ":\n";
        for (const auto& nodo : nodos) {
            std::cout << "Nodo " << nodo.id << " | Relevancia Actual: " 
                      << nodo.relevanciaActual << "\n";
        }
        std::cout << std::endl;

    while (!convergencia) {
        convergencia = true;
        iteracion++;

        // Guardar las relevancias previas y calcular las nuevas
        for (auto& nodo : nodos) {
            nodo.relevanciaPrev = nodo.relevanciaActual;
            nodo.relevanciaActual = 0.0;
        }

        for (const auto& conexion : conexiones) {
            nodos[conexion.destination].relevanciaActual += 
                nodos[conexion.origin].relevanciaPrev * conexion.weight;
        }

        // Verificar convergencia
        for (const auto& nodo : nodos) {
            if (std::abs(nodo.relevanciaActual - nodo.relevanciaPrev) >= epsilon) {
                convergencia = false;
            }
        }

        // Mostrar la relevancia actual en cada iteración (opcional)
        std::cout << "Iteración " << iteracion << ":\n";
        for (const auto& nodo : nodos) {
            std::cout << "Nodo " << nodo.id << " | Relevancia Actual: " 
                      << nodo.relevanciaActual << "\n";
        }
        std::cout << std::endl;
    }

    std::cout << "Convergencia alcanzada en " << iteracion << " iteraciones.\n";

    return 0;
}
