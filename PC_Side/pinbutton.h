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
    void setState(quint8 state, int port, int pin);

public slots:
    void setTxt(int port, int pin, QString text);
    void setColor(quint8 port, quint8 pin, bool color);
    void enableButton(quint8 port_in, quint8 pin_in, bool b);

private:
    void state(quint8 state);
    const QVector<QString> pnamestr = {"PA", "PB", "PC", "PD", "PE", "PF"};
};

#endif // PINBUTTON_H
