/* InputManager - Input Injection (Stub) */
#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <QObject>

class InputManager : public QObject
{
    Q_OBJECT
public:
    explicit InputManager(QObject *parent = nullptr) : QObject(parent) {}
};

#endif
