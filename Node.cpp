#include "Node.h"

Node::Node(int id, double relevanciaActual, double relevanciaPrev)
    : id(id), relevanciaActual(relevanciaActual), relevanciaPrev(relevanciaPrev) {}

void Node::actualizarRelevancia(double nuevaRelevancia) {
    relevanciaPrev = relevanciaActual;
    relevanciaActual = nuevaRelevancia;
}
