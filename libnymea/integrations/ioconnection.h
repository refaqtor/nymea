#ifndef IOCONNECTION_H
#define IOCONNECTION_H

#include <QObject>

#include "typeutils.h"

class IOConnection
{
    Q_GADGET
public:
    IOConnection();
    IOConnection(const ThingId &inputThing, const StateTypeId &inputState, const ThingId &outputThing, const StateTypeId &outputState);

    ThingId inputThing() const;
    StateTypeId inputState() const;

    ThingId outputThing() const;
    StateTypeId outputState() const;

private:
    ThingId m_inputThing;
    StateTypeId m_inputState;
    ThingId m_outputThing;
    StateTypeId m_outputState;
};

#endif // IOCONNECTION_H
