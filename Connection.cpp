#include "Connection.h"

Connection::Connection(int id, int origin, int destination, double weight)
    : id(id), origin(origin), destination(destination), weight(weight) {}

void Connection::actualizarPeso(double nuevoPeso) {
    weight = nuevoPeso;
}
