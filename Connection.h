#ifndef CONNECTION_H
#define CONNECTION_H

class Connection {
public:
    int id;
    int origin;
    int destination;
    double weight;

    Connection() : id(0), origin(0), destination(0), weight(0.0) {}

    Connection(int id, int origin, int destination, double weight);

    void actualizarPeso(double nuevoPeso);
};

#endif // CONNECTION_H
