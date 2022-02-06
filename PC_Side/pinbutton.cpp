#include "pinbutton.h"

PinButton::PinButton()
{

}

PinButton::PinButton(int port_init, int pin_init)
{
    port = port_init;
    pin = pin_init;
    setTxt(port, pin, "Н/Д");
}

void PinButton::state(quint8 state)
{
    emit setState(state, port, pin);
}

void PinButton::setTxt(int port_in, int pin_in, QString text_in)
{
    if (port_in == port && pin_in == pin)
        this->setText(pnamestr.at(port) + QString::number(pin) + " - " + text_in);
}

void PinButton::setColor(quint8 port_in, quint8 pin_in, bool color)
{
    if (port_in == port && pin_in == pin && this->isEnabled()){
        if (color) this->setStyleSheet("QPushButton{background:#00FF00;}");
        else if(!color) this->setStyleSheet("QPushButton{background:#FFFFFF;}");
    }
}

void PinButton::enableButton(quint8 port_in, quint8 pin_in, bool b)
{
    if (port_in == port && pin_in == pin){
        this->setEnabled(b);
        if (!(this->isEnabled()))
        {
            this->setStyleSheet("QPushButton{background:none;}");
            setTxt(port, pin, "Н/Д");
        }
    }
}
