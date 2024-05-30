#include "websocketserver.h"
#include <QDesktopWidget>
#include <QScreen>
#include <QApplication>
#include <QtNetwork>
#include <QSslConfiguration>
#include <QCoreApplication>
#include <QFileInfo>

WebSocketServer::WebSocketServer(QObject *parent) : QObject(parent)
{
    m_webHttpServer = new QWebSocketServer(QStringLiteral("Engage Web Http Server"),QWebSocketServer::NonSecureMode,this);
    m_webHttpsServer = new QWebSocketServer(QStringLiteral("Engage Web Https Server"),QWebSocketServer::SecureMode,this);


    QFile file1(":/server.pem");
    file1.open(QIODevice::ReadOnly);
    QSslCertificate sslCert(&file1,QSsl::Pem);
    file1.close();

    QFile file2(":/server_key.pem");
    file2.open(QIODevice::ReadOnly);
    QSslKey sslKey(&file2,QSsl::Rsa,QSsl::Pem);
    file2.close();

    QSslConfiguration sslConfig;
    sslConfig.setLocalCertificate(sslCert);
    sslConfig.setPrivateKey(sslKey);
    sslConfig.setProtocol(QSsl::AnyProtocol);
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    m_webHttpsServer->setSslConfiguration(sslConfig);

    m_webHttpServer->listen(QHostAddress::Any,m_http_port);
    m_webHttpsServer->listen(QHostAddress::Any,m_https_port);

    connect(m_webHttpServer,&QWebSocketServer::newConnection,this,&WebSocketServer::onNewConnection);
    connect(m_webHttpsServer,&QWebSocketServer::newConnection,this,&WebSocketServer::onNewConnection2);

    connect(this, &WebSocketServer::sigSendMsg, this, &WebSocketServer::slotSendMsg, Qt::QueuedConnection);

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect rect = screen->geometry();
    m_sw = rect.width();
    m_sh = rect.height();
    QObject::connect(screen,&QScreen::geometryChanged,[=](const QRect &newGeometry){
        qDebug()<<" screen width "<< newGeometry.width() <<" height "<< newGeometry.height();
        m_sw = newGeometry.width();
        m_sh = newGeometry.height();
    });

    qDebug()<<"-------------WebSocketServer::start";
}

void WebSocketServer::onNewConnection()
{
    qDebug()<<"-------------WebSocketServer::onNewConnection Http";

    auto client = m_webHttpServer->nextPendingConnection();
    if(client == nullptr) {
        return;
    }


    auto index =  m_http_clientIndex ++;
    client->setProperty("http_clientIndex", QVariant::fromValue(index));
    if(m_http_Clients.contains(index)) {
        m_http_Clients[index]->close();
        m_http_Clients[index]->deleteLater();
        m_http_Clients.remove(index);
    }
    m_http_Clients[index]=client;
    connect(client,&QWebSocket::textMessageReceived,this,&WebSocketServer::processTextMessage);
    connect(client, &QWebSocket::binaryMessageReceived, this, &WebSocketServer::processBinaryMessage);
    connect(client, &QWebSocket::disconnected, this, &WebSocketServer::socketDisconnected);



}

void WebSocketServer::onNewConnection2()
{
    qDebug()<<"-------------WebSocketServer::onNewConnection Https";

    auto client = m_webHttpsServer->nextPendingConnection();
    if(client == nullptr) {
        return;
    }


    auto index =  m_https_clientIndex++;
    client->setProperty("https_clientIndex", QVariant::fromValue(index));
    if(m_https_Clients.contains(index)) {
        m_https_Clients[index]->close();
        m_https_Clients[index]->deleteLater();
        m_https_Clients.remove(index);
    }
    m_https_Clients[index]=client;
    connect(client,&QWebSocket::textMessageReceived,this,&WebSocketServer::processTextMessage);
    connect(client, &QWebSocket::binaryMessageReceived, this, &WebSocketServer::processBinaryMessage);
    connect(client, &QWebSocket::disconnected, this, &WebSocketServer::socketDisconnected);

}


void WebSocketServer::processTextMessage(QString message)
{
    qDebug()<<"-------------WebSocketServer::processTextMessage "<<message;
}

void WebSocketServer::processBinaryMessage(QByteArray message)
{
    bool isWebFocus = message[0] == (uint8_t)(0x01);
    isBoardAcitve = isWebFocus;
    auto client = qobject_cast<QWebSocket*>(sender());
    client->setProperty("isWebFocus",QVariant::fromValue(isWebFocus));
    QCoreApplication::instance()->setProperty("isEngageWebFocus",QVariant::fromValue(isWebFocus));
    qDebug()<<"-------------  WebSocketServer::processBinaryMessage "<<message;

}

void WebSocketServer::socketDisconnected()
{
    qDebug()<<"-------------WebSocketServer::Disconnected";
    auto client = qobject_cast<QWebSocket*>(sender());

    disconnect(client, 0, this, 0);

   if(!m_http_Clients.isEmpty())
   {
       auto index = client->property("http_clientIndex").toULongLong();

        m_http_Clients.remove(index);
   }

   if(!m_https_Clients.isEmpty())
   {
       auto index = client->property("https_clientIndex").toULongLong();
        m_https_Clients.remove(index);
   }
    client->deleteLater();
}

WebSocketServer::~WebSocketServer()
{
    if(m_webHttpServer) {
        if(m_webHttpServer->isListening()) {
            m_webHttpServer->close();
        }
    }

    if(m_webHttpsServer) {
        if(m_webHttpsServer->isListening()) {
            m_webHttpsServer->close();
        }
    }
}


void WebSocketServer::SendData(const QByteArray &data)
{
    emit sigSendMsg(data);
}


QVector<int> state2(2,-1);
QVector<QVector<int>> state_specialTip2(64,state2);
QVector<bool> index_specialTip2(20,false);

void WebSocketServer::slotSendMsg(QByteArray data)
{
    if(!isBoardAcitve) return;

    QByteArray byte;
    int bufferIndex = 0;
    //    qDebug()<<"state_specialTip = "<<state_specialTip;
    //    qDebug()<<"index_specialTip = "<<index_specialTip;
    uint8_t buf[64] = {0};
    char * str = data.data();
    //    qDebug()<<" str = "<< str[0];
    for(int i=0 ; i<64; i++)
    {
        buf[i] = str[i];
        //        qDebug()<<"buf "<<hex<<buf[4];

    }



    for(int i=0 ; i<=5; i++)

    {
//        int tempX = (int)(buf[4+(i*10)]) * 256 + (int)(buf[3+(i*10)]);
//        int tempY = (int)(buf[6+(i*10)]) * 256 + (int)(buf[5+(i*10)]);

//        int tempW = (int)(buf[8+(i*10)]) * 256 + (int)(buf[7+(i*10)]);
//        int tempH = (int)(buf[10+(i*10)]) * 256 + (int)(buf[9+(i*10)]);

        int tempX = (int)(buf[6+(i*10)]) * 256 + (int)(buf[5+(i*10)]);
        int tempY = (int)(buf[8+(i*10)]) * 256 + (int)(buf[7+(i*10)]);

        int tempW = (int)(buf[10+(i*10)]) * 256 + (int)(buf[9+(i*10)]);
        int tempH = (int)(buf[12+(i*10)]) * 256 + (int)(buf[11+(i*10)]);



        if(tempX == 0 && tempY == 0) {break;}

//        int touchID = (int)(buf[2+(i*10)]);
//        int touchState =  buf[1+(i*10)] & 0x0F;
//        int touchType = (buf[1+(i*10)] & 0xF0) >> 4;

        int touchID = (buf[4+(i*10)]);
        int touchState =  buf[3+(i*10)] & 0x0F;
        int touchType = (buf[3+(i*10)] & 0xF0) >> 4;
//        qDebug()<<"touchType = "<< touchType<<"touchState = "<< touchState;
//         qDebug()<<"touchID = "<<touchID;
//        qDebug()<<"X: "<<tempX<<" Y: "<<tempY;
//        qDebug()<<"W: "<<tempW<<" H: "<<tempH;
        if(touchID > 20) break;
        if(touchState == 7)
        {
            if(state_specialTip2[touchID][0] == -1)
            {
                for(int k=0; k <= 19 ; k++)
                {
                    if(index_specialTip2[k] == false)
                    {
                        state_specialTip2[touchID][0] = k;
                        index_specialTip2[k] = true;

                        if(state_specialTip2[touchID][1] == -1)
                        {
                            state_specialTip2[touchID][1] = 0; //touch down
                        }
                        break;
                    }
                }
            }else if(state_specialTip2[touchID][1] == 0)
            {
                state_specialTip2[touchID][1] = 2;//touch down and move
            }
        }else if(touchState == 4)
        {
            if(state_specialTip2[touchID][0] >= 0)
            {
                state_specialTip2[touchID][1] = 1;//touch up
            }
        }

        if(state_specialTip2[touchID][0] >= 0)
        {

            byte[bufferIndex+0] = 0xf3;
            byte[bufferIndex+1] = 0x0e;
            byte[bufferIndex+2] = touchType;
            byte[bufferIndex+3] = (uint8_t)(state_specialTip2[touchID][0] + 1);
            byte[bufferIndex+4] = (uint8_t)(state_specialTip2[touchID][1]);
            // 4k 3840x2160 2k 2560X1600


            tempX = (int)((float)tempX / (float)32767 * m_sw);
            tempY = (int)((float)tempY / (float)32767 * m_sh);
//            qDebug()<<"X2: "<<tempX<<" Y2: "<<tempY;
//            qDebug()<<"touchID: "<<state_specialTip[touchID][0] + 1<<" touchState: "<<state_specialTip[touchID][1];

            byte[bufferIndex+5] = ((tempX)/256);
            byte[bufferIndex+6] = (tempX)%256;
            byte[bufferIndex+7] = (tempY)/256;
            byte[bufferIndex+8] = (tempY)%256;
            byte[bufferIndex+9] = (tempW)/256;
            byte[bufferIndex+10] = (tempW)%256;
            byte[bufferIndex+11] = (tempH)/256;
            byte[bufferIndex+12] = (tempH)%256;
            byte[bufferIndex+13] = 0xf4;
            bufferIndex += 14;

        }


        if(touchState == 4)
        {
            if(state_specialTip2[touchID][0] >= 0)
            {
                index_specialTip2[state_specialTip2[touchID][0]] = false;
                state_specialTip2[touchID][0] = -1;
                state_specialTip2[touchID][1] = -1;
            }
        }
    }

    if(byte.isEmpty()) {
        return;
    }

    if(!m_http_Clients.isEmpty())
    {
        auto itr1 = m_http_Clients.begin();
        while (itr1 != m_http_Clients.end()) {
    //        itr.value()->sendTextMessage(QString::fromLatin1(byte.toHex()));
            if(itr1.value()->property("isWebFocus").toBool())
            {
                itr1.value()->sendBinaryMessage(byte);
            }
            ++itr1;
        }
        qDebug()<<"WebSocketServer::SendData To Web"<<byte.toHex()<<m_http_Clients.size();

    }

    if(!m_https_Clients.isEmpty())
    {
        auto itr2 = m_https_Clients.begin();
        while (itr2 != m_https_Clients.end()) {
    //        itr.value()->sendTextMessage(QString::fromLatin1(byte.toHex()));
            if(itr2.value()->property("isWebFocus").toBool())
            {
                itr2.value()->sendBinaryMessage(byte);
            }

            ++itr2;
        }
        qDebug()<<"WebSocketServer::SendData To Web"<<byte.toHex()<<m_https_Clients.size();

    }


}
