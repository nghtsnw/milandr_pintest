#include "rxparcer.h"
#include <QDebug>

Parcer::Parcer(QObject *parent) : QObject{parent}
{
}

void Parcer::getRawData(QByteArray r_data) // разбираем сырые данные и отдаём на формирование пакета
{    
    for (auto databyte : r_data)
    {
                toQue.insert(0, databyte); // пишем байт в отдельный маленький буфер
                intToQue = toQue.toHex().toInt(&ok, 16); // конвертируем в инт
                getByte(intToQue);                
                toQue.clear();
    }
}

void Parcer::getByte(uint8_t byteFromBuf) // собираем фрейм, при удовлетворении условий отправляем
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
            }
        }
    else if (frameMsg.size() >= 2) frameMsg.dequeue();// а если сигнатура начала пакета не сошлась то сдвигаем очередь
    }
}

uint8_t Parcer::calcCrc(const QVector<uint8_t> &arr)
{
    uint8_t crc = 0;
    for (int i = 0; i < arr.size()-1; i++) crc += arr[i];
    return crc;
}
