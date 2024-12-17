#ifndef NODE_H
#define NODE_H

#pragma pack(1)
class Node {
public:
    int id;
    double relevanciaActual;
    double relevanciaPrev;
    int outCon;
    int inCon;

    // Constructor por defecto
    Node() : id(0), relevanciaActual(0.0), relevanciaPrev(0.0), outCon(0), inCon(0) {}

    // Constructor con parámetros
    Node(int id, double relevanciaActual, double relevanciaPrev, int outCon, int inCon)
    : id(id), relevanciaActual(relevanciaActual), relevanciaPrev(relevanciaPrev), outCon(outCon), inCon(inCon) {}

    void actualizarRelevancia(double nuevaRelevancia);
};
#pragma pack()

#endif // NODE_H
