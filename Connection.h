#ifndef CONNECTION_H
#define CONNECTION_H

#pragma pack(1)
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
#pragma pack()

#endif // CONNECTION_H
