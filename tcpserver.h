#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <unistd.h>
#include <multifingergesture.h>

class TcpServer:public QObject
{
    Q_OBJECT
public:
    TcpServer(QObject*);
    ~TcpServer();
    void SendData(const QByteArray&);
    void setBoardActive(bool);

signals:
    void sigSendMsg(QByteArray);
    void sigSendBoardActive(bool);

public slots:
    void newConnection();
    void readData();
    void disconnected();
    void slotSendMsg(QByteArray);
    void slotReciveBoardActive(bool);

public:
    bool isBoardAcitve = false;

private:
    QTcpServer* m_tcp = nullptr;
    int m_port = 9000;
    QMap<uint64_t,QTcpSocket*> m_Clients;
    uint64_t           m_clientindex = 0;

    float m_sw;// virtual screen width
    float m_sh;// virtual screen height

    MultiFingerGesture *m_gesture = nullptr;

};

#endif // TCPSERVER_H
