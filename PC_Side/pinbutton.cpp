#include "pinbutton.h"

PinButton::PinButton()
{

}

PinButton::PinButton(int port_init, int pin_init)
{
    port = port_init;
    pin = pin_init;
}

void PinButton::state(uint8_t state)
{
    emit setState(state, port, pin);
}

void PinButton::setTxt(QString port_in, int pin_in, QString text_in)
{
    if (port_in == port && pin_in == pin)
        this->setText(port + QString::number(pin) + " - " + text_in);
}

void PinButton::setColor(uint8_t port_in, uint8_t pin_in, bool color)
{
    if (port_in == port && pin_in == pin){
        if (color) this->setStyleSheet("QPushButton{background:#00FF00;}");
        else if(!color) this->setStyleSheet("QPushButton{background:none;}");
    }
}

void PinButton::enableButton(uint8_t port_in, uint8_t pin_in, bool b)
{
    if (port_in == port && pin_in == pin){
        this->setEnabled(b);
    }
}
