#ifndef IOCONNECTION_H
#define IOCONNECTION_H

#include <QObject>

#include "typeutils.h"

class IOConnection
{
    Q_GADGET
public:
    IOConnection();
    IOConnection(const ThingId &inputThing, const StateId &inputState, const ThingId &outputThing, const StateId &outputState);

    ThingId inputThing() const;
    StateId inputState() const;

    ThingId outputThing() const;
    StateId outputState() const;

private:
    ThingId m_inputThing;
    StateId m_inputState;
    ThingId m_outputThing;
    StateId m_outputState;
};

#endif // IOCONNECTION_H
