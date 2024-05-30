#include "tcpserver.h"
#include <QDebug>
#include <QDesktopWidget>
#include <QScreen>
#include <QApplication>

TcpServer::TcpServer(QObject*parent):QObject(parent)
{
    m_tcp = new QTcpServer(this);
    //HostAddress address;
    //address.setAddress("127.0.0.1");

    m_tcp->listen(QHostAddress::Any, m_port);
    connect(m_tcp, &QTcpServer::newConnection,
            this, &TcpServer::newConnection, Qt::QueuedConnection);
    connect(this, &TcpServer::sigSendMsg, this, &TcpServer::slotSendMsg, Qt::QueuedConnection);
    connect(this, &TcpServer::sigSendBoardActive, this, &TcpServer::slotReciveBoardActive, Qt::QueuedConnection);

    
    qDebug()<<"lisnt---------------"<<m_tcp->isListening() << m_tcp->errorString();

    //    qDebug()<<" main screen "<< QGuiApplication::primaryScreen()->geometry().width();
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect rect = screen->geometry();
    m_sw = rect.width();
    m_sh = rect.height();

    QObject::connect(screen,&QScreen::geometryChanged,[=](const QRect &newGeometry){
        qDebug()<<" screen width "<< newGeometry.width() <<" height "<< newGeometry.height();
        m_sw = newGeometry.width();
        m_sh = newGeometry.height();
    });


    m_gesture = new MultiFingerGesture(this);
    connect(this,&TcpServer::sigSendMsg,m_gesture,&MultiFingerGesture::slotSendMsg,Qt::QueuedConnection);


}

TcpServer::~TcpServer()
{
    if(m_tcp) {
        if(m_tcp->isListening()) {
            m_tcp->close();
        }
    }
}

void TcpServer::SendData(const QByteArray &data)
{
    qDebug()<<"-------------TcpServer::SendData";
    emit sigSendMsg(data);
}

void TcpServer::setBoardActive(bool active)
{
    emit sigSendBoardActive(active);
}

void TcpServer::newConnection()
{
    qDebug()<<"-------------TcpServer::newConnection";
    auto client = m_tcp->nextPendingConnection();
    if(client == nullptr) {
        return;
    }
    auto index =  m_clientindex++;
    client->setProperty("clientindex", QVariant::fromValue(index));
    if(m_Clients.contains(index)) {
        m_Clients[index]->close();
        m_Clients[index]->deleteLater();
        m_Clients.remove(index);
    }
    m_Clients[index]=client;
    connect(client, &QTcpSocket::readyRead, this, &TcpServer::readData);
    connect(client, &QTcpSocket::disconnected, this, &TcpServer::disconnected);
}

void TcpServer::readData()
{
    auto client = qobject_cast<QTcpSocket*>(sender());
    auto recvData = client->peek(14);
    qDebug()<<"-------------TcpServer::readData "<< recvData.data();
//    while (recvData.length() == 14) {
//        if(recvData[0] == 0xf3) {
//            if(recvData[13] == 0xf4) {
//                //pack
//                client->read(14);
//            } else {
//                int i=1;
//                while(i < 13 && recvData[i] != 0xf3) {
//                    ++i;
//                }
//                client->read(i+1);
//            }
//        } else {
//            int i=1;
//            while(i < 13 && recvData[i] != 0xf3) {
//                ++i;
//            }
//            client->read(i+1);
//        }
//        recvData = client->peek(14);
//    }

}

void TcpServer::disconnected()
{
    auto client = qobject_cast<QTcpSocket*>(sender());
    auto index = client->property("clientindex").toULongLong();
    disconnect(client, 0, this, 0);
    m_Clients.remove(index);
    client->deleteLater();
}


void TcpServer::slotReciveBoardActive(bool active)
{
    isBoardAcitve = active;
    if(m_gesture != nullptr)
    {
          m_gesture->isBoardAcitve = active;
    }

//    qDebug()<<"slotReciveBoardActive "<<isBoardAcitve;

}

QVector<int> state(2,-1);
QVector<QVector<int>> state_specialTip(64,state);
QVector<bool> index_specialTip(20,false);

void TcpServer::slotSendMsg(QByteArray data)
{
    qDebug()<<"-------------TcpServer::slotSendMsg"<<isBoardAcitve;
    if(!isBoardAcitve) return;

    QByteArray byte;
    int bufferIndex = 0;
    //    qDebug()<<"state_specialTip = "<<state_specialTip;
    //    qDebug()<<"index_specialTip = "<<index_specialTip;
    uint8_t buf[64] = {0};
    char * str = data.data();
    qDebug()<<" str = "<< str[0];
    for(int i=0 ; i<64; i++)
    {
        buf[i] = str[i];
                qDebug()<<"buf i = "<<"value="<<i<<hex<<buf[i];

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

        int touchID = (int)(buf[4+(i*10)]);
        int touchState =  buf[3+(i*10)] & 0x0F;
        int touchType = (buf[3+(i*10)] & 0xF0) >> 4;
//        qDebug()<<"touchType = "<< touchType<<"touchState = "<< touchState;
         qDebug()<<"touchID = "<<touchID<<hex<< " buf: "<<buf[4+(i*10)];
//        qDebug()<<"X: "<<tempX<<" Y: "<<tempY;
//        qDebug()<<"W: "<<tempW<<" H: "<<tempH;
        if(touchState == 7)
        {
            if(state_specialTip[touchID][0] == -1)
            {
                for(int k=0; k <= 19 ; k++)
                {
                    if(index_specialTip[k] == false)
                    {
                        state_specialTip[touchID][0] = k;
                        index_specialTip[k] = true;

                        if(state_specialTip[touchID][1] == -1)
                        {
                            state_specialTip[touchID][1] = 0; //touch down
                        }
                        break;
                    }
                }
            }else if(state_specialTip[touchID][1] == 0)
            {
                state_specialTip[touchID][1] = 2;//touch down and move
            }
        }else if(touchState == 4)
        {
            if(state_specialTip[touchID][0] >= 0)
            {
                state_specialTip[touchID][1] = 1;//touch up
            }
        }

        if(state_specialTip[touchID][0] >= 0)
        {

            byte[bufferIndex+0] = 0xf3;
            byte[bufferIndex+1] = 0x0e;
            byte[bufferIndex+2] = touchType;
            byte[bufferIndex+3] = (uint8_t)(state_specialTip[touchID][0] + 1);
            byte[bufferIndex+4] = (uint8_t)(state_specialTip[touchID][1]);
            // 4k 3840x2160 2k 2560X1600


//            tempX = (int)((float)tempX / (float)32767 * m_sw);
//            tempY = (int)((float)tempY / (float)32767 * m_sh);
            qDebug()<<"X2: "<<tempX<<" Y2: "<<tempY;
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
            if(state_specialTip[touchID][0] >= 0)
            {
                index_specialTip[state_specialTip[touchID][0]] = false;
                state_specialTip[touchID][0] = -1;
                state_specialTip[touchID][1] = -1;
            }
        }
    }

    if(m_Clients.isEmpty()) {
        return;
    }
    auto itr = m_Clients.begin();
    while (itr != m_Clients.end()) {
        itr.value()->write(byte);
        ++itr;
    }
//    qDebug()<<"TcpServer::SendData end"<<byte.length()<<m_Clients.size();

}



