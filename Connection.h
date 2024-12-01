#ifndef CONNECTION_H
#define CONNECTION_H

class Connection {
public:
    int id;
    int origin;
    int destination;
    double weight;

    Connection(int id, int origin, int destination, double weight);

    void actualizarPeso(double nuevoPeso);
};

#endif // CONNECTION_H
