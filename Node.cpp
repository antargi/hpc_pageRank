#include "Node.h"

void Node::actualizarRelevancia(double nuevaRelevancia) {
    relevanciaPrev = relevanciaActual;
    relevanciaActual = nuevaRelevancia;
}
