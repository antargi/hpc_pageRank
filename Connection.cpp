#include "Connection.h"

Connection::Connection(int id, int origin, int destination, float weight)
    : id(id), origin(origin), destination(destination), weight(weight) {}

void Connection::actualizarPeso(float nuevoPeso) {
    weight = nuevoPeso;
}
