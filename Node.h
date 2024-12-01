#ifndef NODE_H
#define NODE_H

class Node {
public:
    int id;
    double relevanciaActual;
    double relevanciaPrev;

    Node(int id, double relevanciaActual = 0.0, double relevanciaPrev = 0.0);
    void actualizarRelevancia(double nuevaRelevancia);
};

#endif // NODE_H
