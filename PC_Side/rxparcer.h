#ifndef RXPARCER_H
#define RXPARCER_H

#include <QObject>
#include <QQueue>

class Parcer : public QObject
{
    Q_OBJECT
public:
    explicit Parcer(QObject *parent = nullptr);

signals:
    void deviceData(QVector<uint8_t> snapshot);

public slots:
    void getRawData(QByteArray r_data);    

private:
    void getByte(uint8_t byteFromBuf);
    QByteArray toQue;
    int intToQue = 0;
    const int oneMsgLeight = 40;
    QQueue<uint8_t> frameMsg;
    QVector<uint8_t> snapshot;
    bool ok;
    uint8_t calcCrc(const QVector<uint8_t> &arr);
};

#endif // RXPARCER_H
