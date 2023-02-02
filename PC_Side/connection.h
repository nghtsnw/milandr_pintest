#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QQueue>
#include <QSerialPort>
#include <QTimer>

class Connection : public QObject
{
    Q_OBJECT
public:
    QTimer *mcu_wdt = new QTimer(this);
    QTimer *response_wdt = new QTimer(this);
    explicit Connection(QObject *parent = nullptr);
    QSerialPort *m_serial = nullptr;
    void writeData(const QByteArray &data);
    ~Connection();

signals:
    void deviceData(QVector<quint8> snapshot);
    void resetConnection();

public slots:
    void getRawData(QByteArray r_data);
    void readData();
    void receiveCommand(QVector<quint8> command, bool newcommandflag);
    void responseWaitingTimer(int time, bool start);
    void mcuMessageTimer(int time, bool start);
    void stopBeforeQuit();
    void closePort();

private:    
    void getByte(quint8 byteFromBuf);
    QByteArray toQue;
    int intToQue = 0;
    const int oneMsgLeight = 16;
    QQueue<quint8> frameMsg;
    QVector<quint8> snapshot;
    bool ok;
    quint8 calcCrc(const QVector<quint8> &arr);
    bool newcommand = false;
    QVector<quint8> toTransmit;

private slots:
    void sendCommand();

};

#endif // CONNECTION_H
