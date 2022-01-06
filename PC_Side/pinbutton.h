#ifndef PINBUTTON_H
#define PINBUTTON_H

#include <QPushButton>
#include <QObject>
#include <QWidget>

class PinButton : public QPushButton
{
    Q_OBJECT
public:
    PinButton();
    PinButton(int port_init, int pin_init);
    int port;
    int pin;

signals:
    void setState(uint8_t state, int port, int pin);

public slots:
    void setTxt(QString port, int pin, QString text);
    void setColor(uint8_t port, uint8_t pin, bool color);
    void enableButton(uint8_t port_in, uint8_t pin_in, bool b);

private:
    void state(uint8_t state);
};

#endif // PINBUTTON_H
