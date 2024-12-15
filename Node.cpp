#include "Node.h"

void Node::actualizarRelevancia(float nuevaRelevancia) {
    relevanciaPrev = relevanciaActual;
    relevanciaActual = nuevaRelevancia;
}
