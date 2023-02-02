#include "connection.h"
#include <QDebug>
#include <QVector>
#include <QThread>
#include <QSerialPort>

Connection::Connection(QObject *parent) : QObject{parent}
{
    m_serial = new QSerialPort(this);
    connect(m_serial, &QSerialPort::readyRead, this, &Connection::readData);
    connect(mcu_wdt, &QTimer::timeout, this, &Connection::sendCommand); // Тут сыпется релиз при подключении
    connect(response_wdt, &QTimer::timeout, this, [=]{emit resetConnection();});
}

Connection::~Connection()
{
}

void Connection::getRawData(QByteArray r_data) // разбираем сырые данные и отдаём на формирование пакета
{    
    for (auto databyte : r_data)
    {
                toQue.insert(0, databyte); // пишем байт в отдельный маленький буфер
                intToQue = toQue.toHex().toInt(&ok, 16); // конвертируем в инт
                getByte(intToQue);                
                toQue.clear();
    }
}

void Connection::getByte(quint8 byteFromBuf) // собираем фрейм, при удовлетворении условий отправляем
{    
    frameMsg.enqueue(byteFromBuf);
    if (frameMsg.size() >= 3) // условие начала пакета
    {
        if ((frameMsg[0] == 0xFF) && (frameMsg[1]==0xFF) && ((frameMsg[2]==0x01) || (frameMsg[2]==0x02)|| (frameMsg[2]==0x03)))
        {
            if (frameMsg.size() == oneMsgLeight)
            {
                snapshot = frameMsg.toVector();                
                if (calcCrc(snapshot) == snapshot.last())
                emit deviceData(snapshot); // если пакет сформирован, отправляем пакет в гуй и обнуляем буфер
                frameMsg.clear();
                responseWaitingTimer(3000, true); // Откладываем таймер ожидания корректного пакета ещё на три секунды
            }
        }
    else if (frameMsg.size() >= 2) frameMsg.dequeue();// а если сигнатура начала пакета не сошлась то сдвигаем очередь
    }
}

void Connection::writeData(const QByteArray &data)
{
    m_serial->write(data);
}

void Connection::readData()
{
    const QByteArray data = m_serial->readAll();
    getRawData(data);
}

void Connection::sendCommand()
{ // Отправляем команду контроллеру
    if (m_serial->isOpen()){
        if (!newcommand) toTransmit = {0xFF, 0xA1, 0, 0, 0};
        toTransmit.last() = calcCrc(toTransmit);
        QByteArray ba;
        for (auto i : qAsConst(toTransmit))
            ba.append(i);
        writeData(ba);
        if (newcommand) newcommand = false;
    }
}

void Connection::receiveCommand(QVector<quint8> command, bool newcommandflag)
{ // Тут принимаем команду сформированную в GUI
    toTransmit = command;
    newcommand = newcommandflag;
}

void Connection::responseWaitingTimer(int time, bool start)
{
    if (start) response_wdt->start(time);
    else response_wdt->stop();
}

void Connection::mcuMessageTimer(int time, bool start)
{
    if (start) mcu_wdt->start(time);
    else mcu_wdt->stop();
}

void Connection::stopBeforeQuit()
{
    response_wdt->stop();
    mcu_wdt->stop();
}

void Connection::closePort()
{
    m_serial->close();
}

quint8 Connection::calcCrc(const QVector<quint8> &arr)
{
    quint8 crc = 0;
    for (int i = 0; i < arr.size()-1; i++) crc += arr[i];
    return crc;
}
