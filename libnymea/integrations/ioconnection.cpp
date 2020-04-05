#include "ioconnection.h"

IOConnection::IOConnection()
{

}

IOConnection::IOConnection(const ThingId &inputThing, const StateId &inputState, const ThingId &outputThing, const StateId &outputState):
    m_inputThing(inputThing),
    m_inputState(inputState),
    m_outputThing(outputThing),
    m_outputState(outputState)
{

}

ThingId IOConnection::inputThing() const
{
    return m_inputThing;
}

StateId IOConnection::inputState() const
{
    return m_inputState;
}

ThingId IOConnection::outputThing() const
{
    return m_outputThing;
}

StateId IOConnection::outputState() const
{
    return m_outputState;
}
