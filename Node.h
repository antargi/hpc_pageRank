#ifndef NODE_H
#define NODE_H

#pragma pack(1)
class Node {
public:
    int id;
    double relevanciaActual;
    double relevanciaPrev;

    Node() : id(0), relevanciaActual(0.0), relevanciaPrev(0.0) {}

    Node(int id, double relevanciaActual = 0.0, double relevanciaPrev = 0.0)
        : id(id), relevanciaActual(relevanciaActual), relevanciaPrev(relevanciaPrev) {}

    void actualizarRelevancia(double nuevaRelevancia);
};
#pragma pack()

#endif // NODE_H
