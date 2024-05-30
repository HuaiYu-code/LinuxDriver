#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QHostAddress>
#include <QByteArray>
#include <unistd.h>

class WebSocketServer : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketServer(QObject *parent = 0);
    ~WebSocketServer();
    void SendData(const QByteArray&);

signals:

    void sigSendMsg(QByteArray);

public slots:
    void slotSendMsg(QByteArray);

private slots:
    void onNewConnection();
    void onNewConnection2();

    void processTextMessage(QString message);
    void processBinaryMessage(QByteArray message);
    void socketDisconnected();


public:
    bool isBoardAcitve = false;

private:

    QWebSocketServer* m_webHttpServer;
    QWebSocketServer* m_webHttpsServer;

    int m_http_port = 9001;
    int m_https_port = 9002;
    QMap<uint64_t,QWebSocket*> m_http_Clients;
    QMap<uint64_t,QWebSocket*> m_https_Clients;

    uint64_t           m_http_clientIndex = 0;
    uint64_t           m_https_clientIndex = 0;

    float m_sw;// virtual screen width
    float m_sh;// virtual screen height
};

#endif // WEBSOCKETSERVER_H
