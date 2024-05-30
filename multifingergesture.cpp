#include "multifingergesture.h"
#include <QDateTime>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysymdef.h>
#include <X11/extensions/XTest.h>
#include <QPoint>
#include <unistd.h>

#include <string>
#include <iostream>
#include <sys/types.h>
#include <QCoreApplication>
#include <QScreen>
#include <QGuiApplication>

Display *display;


MultiFingerGesture::MultiFingerGesture(QObject *parent) :
    QObject(parent)
{
    // single touch process
    QVector<int> ele(3,0);
    QVector<QVector<int>> buf(arrayBufferMax+1,ele);
    QVector<QVector<QVector<int>>> finger(touchFingerMax+1,buf);
    m_finger = finger;

    // tow finger process
    QVector<int> t(2,0);
    scrollPreviousX = t;
    scrollPreviousY = t;
    QVector<QVector<int>> tfXY(2,t);
    tfBeginXY = tfXY;

    // three finger process
    QVector<int> tr(3,0);
    threeFingerDeltax = tr;
    QVector<QVector<int>> thfXY(3,t);
    thfBeginXY = thfXY;

    // four finger process
    QVector<int> ftr(5,0);
    QVector<QVector<int>> ffXY(5,t);
    ffBeginXY = ffXY;
    fourFingerDeltaX = ftr;
    fourFingerDeltaY = ftr;

    // five finger process
    QVector<int> fdis(10,0);
    beginFiveFingerDistance = fdis;

    timer = new QTimer(this);
    timer->setInterval(100);
    timer->start();
    connect(timer,&QTimer::timeout,this,&MultiFingerGesture::slotTimerAction);

    display = XOpenDisplay(NULL);

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect rect = screen->virtualGeometry();
    m_sw = rect.width();
    m_sh = rect.height();
    qDebug()<<"first read screen width "<< rect.width() <<" height "<< rect.height();

    QObject::connect(screen,&QScreen::geometryChanged,[=](const QRect &newGeometry){
        qDebug()<<" screen width height change "<< newGeometry.width() <<" height "<< newGeometry.height();
        m_sw = newGeometry.width();
        m_sh = newGeometry.height();
    });



}

MultiFingerGesture::MultiFingerGesture(QByteArray data,int length,QObject *parent) :
    QObject(parent),
    m_data(data),
    m_length(length)
{

    QVector<int> ele(3,0);
    QVector<QVector<int>> buf(arrayBufferMax+1,ele);
    QVector<QVector<QVector<int>>> finger(touchFingerMax+1,buf);
    m_finger = finger;

}

void MultiFingerGesture::slotTimerAction()
{

    //qDebug()<<"MultiFingerGesture::slotTimerAction start";
    if(lastACTF == actionCountForTimer)
    {
        m_bufferIndex = 0;
        scrollEventCount = 0;
        cancelLeftButton = false;
        cancelTwoFinger = false;

//        {
//            int ctrl_keycode = XKeysymToKeycode(display,XStringToKeysym("Control_L"));
//            keyUp(ctrl_keycode);
//            ctrlKeyDownFlag = false;
//        }

//        if(shiftKeyDownFlag)
//        {
//            int shift_keycode = XKeysymToKeycode(display,XStringToKeysym("Shift_L"));
//            if(ctrlKeyDownFlag)
//                keyUp(shift_keycode);
//            shiftKeyDownFlag = false;
//        }

        twoBeginFlag = false;
        threeBeginFlag = false;
        fourBeginFlag = false;
        gestureDone = false;
    }else
    {
        lastACTF = actionCountForTimer;
    }
}

void MultiFingerGesture::slotSendMsg(QByteArray data)
{

    qDebug()<<"MultiFingerGesture::slotSendMsg " << data.data();
    m_data = data;
    parseInfraredFrame();
    actionCountForTimer +=1;
    if(actionCountForTimer >= 10000)
    {
        actionCountForTimer = 0;
    }
    qDebug()<<"fingerNum" <<fingerNum;
    isBoardAcitve = isBoardAcitve || QCoreApplication::instance()->property("isEngageWebFocus").toBool();

    if(fingerNum == 1 && !ctrlKeyDownFlag && !gestureDone)
    {
        singleTouchProcess();
    }else if(fingerNum == 2 && !cancelTwoFinger && !ctrlKeyDownFlag &&!gestureDone)
    {
        cancelLeftButton = true;
        twoFingerProcess();
    }else if(fingerNum == 3 && !isBoardAcitve)
    {
        cancelLeftButton = true;
        cancelTwoFinger = true;
        threeFingerProcess();
    }else if(fingerNum == 4 && !gestureDone && !isBoardAcitve)
    {
        cancelLeftButton = true;
        cancelTwoFinger = true;
        fourFingerProcess();
    }
    else if(fingerNum == 5  && !ctrlKeyDownFlag && !gestureDone && !isBoardAcitve)

    {
        cancelLeftButton = true;
        cancelTwoFinger = true;
        fourFingerProcess();
    }
    last1Tap = finger1Tap;
    last2Tap = finger2Tap;
    lastFingerNum = fingerNum;
}

void MultiFingerGesture::parseInfraredFrame()
{
    qDebug()<<"MultiFingerGesture::parseInfraredFrame ";
    uint8_t buf[64] = {0};
    char * str = m_data.data();
    for(int i=0 ; i<64; i++)
    {
        buf[i] = str[i];

    }



    if(buf[63]>0)
    {
        fingerNum = (int)buf[63];
        packetCountTenFinger = 0;
    }
    else
    {
        packetCountTenFinger +=1;
    }

    //Array is full ,so remove the first packet and add a new one
    if(m_bufferIndex == arrayBufferMax)
    {
        if(fingerNum > 6)
        {
            if(packetCountTenFinger == 0)
            {
                for(int i = 0;i < touchFingerMax; i++)
                {
                    m_finger[i].removeFirst();
                    QVector<int> ele(3,0);
                    m_finger[i].insert(arrayBufferMax,ele);
                }

            }

        }
    }

    int tempX = 0;
    int tempY = 0;
    qDebug()<<".......fingerNum:"<<fingerNum;

    for(int i = 0;i < 5; i++)
    {
        int index = i + packetCountTenFinger * 10;
        if(index >= 0 && index <= touchFingerMax)
        {

            tempX = (int)(buf[6+(i*10)]) * 256 + (int)(buf[5+(i*10)]);
            tempY = (int)(buf[8+(i*10)]) * 256 + (int)(buf[7+(i*10)]);
            int touchType = (buf[3+(i*10)] & 0xF0) >> 4;

            if((tempX > 0 || tempY < 0) && touchType !=3)
            {
                m_finger[index][m_bufferIndex][0] = buf[3+(i*10)] & 0x0F;
                m_finger[index][m_bufferIndex][1] = tempX;
                m_finger[index][m_bufferIndex][2] = tempY;
            }
            else
            {
                m_finger[index][m_bufferIndex][0] = 0;
                m_finger[index][m_bufferIndex][1] = 0;
                m_finger[index][m_bufferIndex][2] = 0;
            }

        }
    }

    finger1Tap =  m_finger[0][m_bufferIndex][0] &0x0F;
    finger2Tap = m_finger[1][m_bufferIndex][0] &0x0F;
//    qDebug()<<"MultiFingerGesture::singleTouchProcess ";


}

void MultiFingerGesture::singleTouchProcess()
{
    qDebug()<<"MultiFingerGesture::singleTouchProcess ";
    int x = m_finger[0][m_bufferIndex][1];
    int y = m_finger[0][m_bufferIndex][2];


    QPoint p0 = pointCalibProcess(x,y);
    x = p0.x();
    y = p0.y();
    qDebug()<<"singleTouchProcess x:"<<p0.x()<<" y:"<<p0.y();
    //qDebug()<<"rightClickFlag:"<<rightClickFlag<<" finger1Tap:"<<finger1Tap<<" finger2Tap:"<<finger2Tap<<" lastFingerNum"<<lastFingerNum;
    if(rightClickFlag && (finger1Tap == 7) && (lastFingerNum == 2))
    {
        // call mouse right key
        mouseDown(3);
        mouseUp(3);
        rightClickFlag = false;
        //qDebug()<<"call mouse right key ";

    }

    if(cancelLeftButton)
    {
        if(p0.x()>0|| p0.y()>0)
        {

            qDebug()<<"call mouse move ";
            mouseMove(x,y);

            cancelLeftButton = false;
        }
    }
    else
    {

        if(last1Tap != 7 && finger1Tap == 7)
        {
            // mouse left single click
            leftButtonDownFlag = true;

            qDebug()<<"call mouse move ";
            //qDebug()<<"call mouse down ";
            mouseMove(x,y);
            mouseDown(1);

        }
        else if(last1Tap == 7 && finger1Tap == 7 )
        {
            // mouse drag
            mouseDrag(x,y);

           // qDebug()<<"call mouse drag ";

           // qDebug()<<"call mouse left key ";


        }else if(last1Tap != 4 && finger1Tap == 4)
        {

            mouseUp(1);

            leftButtonDownFlag = false;
            // mou se up

            // mouse  up
            //            QDateTime current_time = QDateTime::currentDateTime();
            //            double current_time_value = current_time.toMSecsSinceEpoch()/1000.0;
            //            if((current_time_value - lastClickTime) < 0.2)
            //            {
            //                qDebug()<<"current_time_value: "<<current_time_value-lastClickTime;
            //                //mouseClick(1);
            //                // mouse down

            //                // mouse up


            //            }
            //            lastClickTime = current_time_value;
        }
    }


    //qDebug()<<"last1Tap: "<<last1Tap<<" finger1Tap: "<<finger1Tap;

}

void MultiFingerGesture::twoFingerProcess()
{
    //qDebug()<<"MultiFingerGesture::twoFingerProcess ";
    //qDebug()<<"MultiFingerGesture::twoFingerProcess"<<"rightClickFlag:"<<rightClickFlag<<" finger1Tap:"<<finger1Tap<<" finger2Tap:"<<finger2Tap<<" lastFingerNum"<<lastFingerNum;

    int x = m_finger[0][m_bufferIndex][1];
    int y = m_finger[0][m_bufferIndex][2];
    QPoint p0 = pointCalibProcess(x,y);
    x = p0.x();
    y = p0.y();
    if(leftButtonDownFlag)
    {
        leftButtonDownFlag = false;
        // mouse up
        mouseUp(1);

    }

    // right click
    if(last2Tap != 7 && finger2Tap == 7)
    {
        QDateTime current_time = QDateTime::currentDateTime();
        finger2TapDownTime = current_time.toMSecsSinceEpoch()/1000.0;

    }else if(last2Tap != 4 && finger2Tap == 4)
    {
        QDateTime current_time = QDateTime::currentDateTime();
        double current_time_value = current_time.toMSecsSinceEpoch()/1000.0;
        if((current_time_value - finger2TapDownTime) < 0.2 && finger1Tap == 7)
        {

            rightClickFlag = true ;

        }
    }


    // Two Finger Gesture
    if((fingerNum == 2) && !rightClickFlag && !isBoardAcitve)
    {
        if(!twoBeginFlag)
        {
            twoBeginFlag = true;
            for(int i = 0;i<2;i++)
            {
                tfBeginXY[i][0] = m_finger[i][m_bufferIndex][1];
                tfBeginXY[i][1] = m_finger[i][m_bufferIndex][2];
                scrollPreviousX[i] = m_finger[i][m_bufferIndex][1];
                scrollPreviousY[i] = m_finger[i][m_bufferIndex][2];
            }

            deltaXBegin = tfBeginXY[0][0] - tfBeginXY[1][0];
            deltaYBegin = tfBeginXY[0][1] - tfBeginXY[1][1];
            absDeltaXBegin = qAbs(deltaXBegin);
            absDeltaYBegin = qAbs(deltaYBegin);


            beginTwoFingerDistance = sqrt(pow((double)deltaXBegin,2.0) + pow((double)deltaYBegin,2));
        }

        QVector<int> e(2,0);
        QVector<QVector<int>> tempXY(2,e);

        for(int i = 0;i<2;i++)
        {
            tempXY[i][0] = m_finger[i][m_bufferIndex][1];
            tempXY[i][1] = m_finger[i][m_bufferIndex][2];

        }

        int deltaXTemp = tempXY[0][0] - tempXY[1][0];
        int deltaYTemp = tempXY[0][1] - tempXY[1][1];
        int absDeltaXTemp = qAbs(deltaXTemp);
        int absDeltaYTemp = qAbs(deltaYTemp);

        int directionX = absDeltaXTemp - absDeltaXBegin;
        int directionY = absDeltaYTemp - absDeltaYBegin;
        int tempDistance = sqrt(pow((double)deltaXTemp,2.0) + pow((double)deltaYTemp,2));
        int deltaDis = tempDistance - beginTwoFingerDistance;
        //         qDebug()<<"deltaXBegin:"<<deltaXBegin<<" deltaYBegin:"<<deltaYBegin;
        //         qDebug()<<"absDeltaXBegin:"<<absDeltaXBegin<<" absDeltaYBegin:"<<absDeltaYBegin;
        //         qDebug()<<"directionX:"<<directionX<<" directionY:"<<directionY;

        //rotate

        if((directionX > 500) && directionY < -500)
        {
            if((deltaXBegin > 0 && deltaYBegin < 0) || (deltaXBegin < 0 && deltaYBegin > 0))
            {
                gestureDone = true;
                // turn right  call Ctrl+R
                key_Ctrl_R();


            }else if((deltaXBegin > 0 && deltaYBegin > 0) || (deltaXBegin < 0 && deltaYBegin < 0))
            {
                gestureDone = true;
                // turn left call Shift+Ctrl+R
                key_Shift_Ctrl_R();

            }

        }else if(directionX < -500 && directionY > 500)
        {
            if((deltaXBegin > 0 && deltaYBegin < 0) || (deltaXBegin < 0 && deltaYBegin > 0))
            {
                gestureDone = true;
                // turn left
                key_Ctrl_R();


            }else if((deltaXBegin > 0 && deltaYBegin > 0) || (deltaXBegin < 0 && deltaYBegin < 0))
            {
                gestureDone = true;
                //turn right
                key_Shift_Ctrl_R();
            }

        }


//        // zoom

        if((deltaDis > 600) && (absDeltaXBegin < absDeltaXTemp) && (absDeltaYBegin < absDeltaYTemp) && (scrollEventCount < 10) && !isBoardAcitve)
        {
            gestureDone = true;
            beginTwoFingerDistance = tempDistance;
            // zoom in
            key_Ctrl_Plus();

        }else if(deltaDis < -600 && (absDeltaXBegin > absDeltaXTemp) &&(absDeltaYBegin > absDeltaYTemp) && scrollEventCount < 10 && !isBoardAcitve)
        {
            gestureDone = true;
            beginTwoFingerDistance = tempDistance;
            // zoom out
            key_Ctrl_Minus();


        }

        // Mouse Scroll
        if(deltaDis < 300 && deltaDis > -300)
        {
            int deltaX = scrollPreviousX[0] - m_finger[0][m_bufferIndex][1];
            int deltaX2 = scrollPreviousX[1] - m_finger[1][m_bufferIndex][1];

            int deltaY = scrollPreviousY[0] - m_finger[0][m_bufferIndex][2];
            int deltaY2 = scrollPreviousY[1] - m_finger[1][m_bufferIndex][2];

            deltaX = (deltaX + deltaX2)/2;
            deltaY = (deltaY + deltaY2)/2;

            if(deltaX > 200)
            {
                scrollEventCount += 1;
                scrollPreviousX[0] = m_finger[0][m_bufferIndex][1];
                scrollPreviousX[1] = m_finger[1][m_bufferIndex][1];
                // scroll to right
                //                mouseDown(7);
                //                mouseUp(7);
            }else if(deltaX < -200)
            {
                scrollEventCount += 1;
                scrollPreviousX[0] = m_finger[0][m_bufferIndex][1];
                scrollPreviousX[1] = m_finger[1][m_bufferIndex][1];
                // scroll to left
                //                mouseDown(6);
                //                mouseUp(6);
            }

            if(deltaY > 200)
            {
                scrollEventCount += 1;
                scrollPreviousY[0] = m_finger[0][m_bufferIndex][2];
                scrollPreviousY[1] = m_finger[1][m_bufferIndex][2];
                // scroll to up
                mouseDown(5);
                mouseUp(5);

            }else if(deltaY < -200)
            {
                scrollEventCount += 1;
                scrollPreviousY[0] = m_finger[0][m_bufferIndex][2];
                scrollPreviousY[1] = m_finger[1][m_bufferIndex][2];
                // scroll to down
                mouseDown(4);
                mouseUp(4);
            }
        }

    }

}

void MultiFingerGesture::threeFingerProcess()
{
   // qDebug()<<"threeFingerProcess";
    if(leftButtonDownFlag)
    {
        leftButtonDownFlag = false;
        mouseUp(1);
    }

    if(!threeBeginFlag)
    {
        threeBeginFlag = true;
        for(int i = 0; i <3; i++)
        {
            thfBeginXY[i][0] = m_finger[i][m_bufferIndex][1];
            thfBeginXY[i][1] = m_finger[i][m_bufferIndex][2];
        }
    }

    // swipe process
    int leftCount = 0;
    int rightCount = 0;
    for(int i = 0; i <3; i++)
    {
        int deltax = m_finger[i][m_bufferIndex][1] - thfBeginXY[i][0];
        threeFingerDeltax[i] = deltax;

        if( deltax > 500)
        {
            leftCount += 1;
        }else if(deltax < -500)
        {
            rightCount += 1;
        }
    }

//    qDebug()<<leftCount<<rightCount;
    if(leftCount == 3)
    {
        gestureDone = true;
        // web browser  previous page
        key_Alt_Left();

    }else if(rightCount == 3)
    {
        gestureDone = true;

        // web browser next page
        key_Alt_Right();
    }


}


void MultiFingerGesture::fourFingerProcess()
{
    if(leftButtonDownFlag)
    {
        leftButtonDownFlag = false;
        mouseUp(1);
    }

    // if 4 finger in then 5 finger in, switch to five finger mode
    if(lastFingerNum == 4 && fingerNum == 5 && beginFingerNum == 4)
    {
        beginFingerNum = fingerNum;
        fourBeginFlag = false;
    }

    if(!fourBeginFlag)
    {
        //       if(beginFingerNum == 4)
        //       {

        //       }

        fourBeginFlag = true;
        beginFingerNum = fingerNum;

        for(int i = 0; i < 5; i ++)
        {
            ffBeginXY[i][0] = m_finger[i][m_bufferIndex][1];
            ffBeginXY[i][1] = m_finger[i][m_bufferIndex][2];

        }


        if(beginFingerNum == 5)
        {
            int x1 = ffBeginXY[0][0];
            int y1 = ffBeginXY[0][1];

            int x2 = ffBeginXY[1][0];
            int y2 = ffBeginXY[1][1];

            int x3 = ffBeginXY[2][0];
            int y3 = ffBeginXY[2][1];

            int x4 = ffBeginXY[3][0];
            int y4 = ffBeginXY[3][1];

            int x5 = ffBeginXY[4][0];
            int y5 = ffBeginXY[4][1];

            beginFiveFingerDistance[0] = calculateTwoPointDistance(x1,y1,x2,y2);
            beginFiveFingerDistance[1] = calculateTwoPointDistance(x1,y1,x3,y3);
            beginFiveFingerDistance[2] = calculateTwoPointDistance(x1,y1,x4,y4);
            beginFiveFingerDistance[3] = calculateTwoPointDistance(x1,y1,x5,y5);
            beginFiveFingerDistance[4] = calculateTwoPointDistance(x2,y2,x3,y3);
            beginFiveFingerDistance[5] = calculateTwoPointDistance(x2,y2,x4,y4);
            beginFiveFingerDistance[6] = calculateTwoPointDistance(x2,y2,x5,y5);
            beginFiveFingerDistance[7] = calculateTwoPointDistance(x3,y3,x4,y4);
            beginFiveFingerDistance[8] = calculateTwoPointDistance(x3,y3,x5,y5);
            beginFiveFingerDistance[9] = calculateTwoPointDistance(x4,y4,x5,y5);

            beginAvgDis = 0;
            for(int i = 0; i < 10; i ++)
            {
                beginAvgDis += beginFiveFingerDistance[i];
            }
            beginAvgDis  /= 10;
        }


    }



    if(beginFingerNum == 5 && fingerNum == 5)
    {
        QVector<int> t(2,0);
        QVector<QVector<int>> tempXY(5,t);
        QVector<int> tempDistance(10,0);

        for(int i = 0; i < 5; i++)
        {
            tempXY[i][0] = m_finger[i][m_bufferIndex][1];
            tempXY[i][1] = m_finger[i][m_bufferIndex][2];
        }

        int x1 = tempXY[0][0];
        int y1 = tempXY[0][1];

        int x2 = tempXY[1][0];
        int y2 = tempXY[1][1];

        int x3 = tempXY[2][0];
        int y3 = tempXY[2][1];

        int x4 = tempXY[3][0];
        int y4 = tempXY[3][1];

        int x5 = tempXY[4][0];
        int y5 = tempXY[4][1];

        tempDistance[0] = calculateTwoPointDistance(x1,y1,x2,y2);
        tempDistance[1] = calculateTwoPointDistance(x1,y1,x3,y3);
        tempDistance[2] = calculateTwoPointDistance(x1,y1,x4,y4);
        tempDistance[3] = calculateTwoPointDistance(x1,y1,x5,y5);
        tempDistance[4] = calculateTwoPointDistance(x2,y2,x3,y3);
        tempDistance[5] = calculateTwoPointDistance(x2,y2,x4,y4);
        tempDistance[6] = calculateTwoPointDistance(x2,y2,x5,y5);
        tempDistance[7] = calculateTwoPointDistance(x3,y3,x4,y4);
        tempDistance[8] = calculateTwoPointDistance(x3,y3,x5,y5);
        tempDistance[9] = calculateTwoPointDistance(x4,y4,x5,y5);

        int tempAvgDis = 0;
        for(int i = 0; i < 10; i ++)
        {
            tempAvgDis += tempDistance[i];
        }
        tempAvgDis  /= 10;

        tempAvgDis -= beginAvgDis;

        if(tempAvgDis > 300 && tempAvgDis < 2000)
        {
            //key_Ctrl_Alt_D();
            key_Super_D();

        }else if(tempAvgDis < -300 && tempAvgDis > -2000)
        {

            // key_Ctrl_Alt_D();
            key_Super_D();
        }
    }

    //    qDebug()<<"fourFingerProcess begingFingerNum = "<<beginFingerNum;
    if(beginFingerNum == 4)
    {
        int upCount = 0;
        int downCount = 0;
        int leftCount = 0;
        int rightCount = 0;

        for(int i = 0; i < 4; i++)
        {
            int ffDeltaX = m_finger[i][m_bufferIndex][1] - ffBeginXY[i][0];
            int ffDeltaY = m_finger[i][m_bufferIndex][2] - ffBeginXY[i][1];
            fourFingerDeltaX[i] = ffDeltaX;
            fourFingerDeltaY[i] = ffDeltaY;


            if(ffDeltaX > 1000)
            {
                leftCount += 1;
            }else if(ffDeltaX < -1000 )
            {
                rightCount += 1;
            }

            if(ffDeltaY > 1000)
            {
                downCount += 1;
            }else if(ffDeltaY < -1000)
            {
                upCount += 1;
            }

            //          qDebug()<<"ffDeltaX :"<<ffDeltaX<<"ffDeltaY:"<<ffDeltaY;

        }

        if(upCount == 4)
        {
            key_Super();
        }else if(downCount == 4)
        {
            key_Super();

        }else if(leftCount == 4)
        {
            key_Ctrl_Alt_Left();

        }else if(rightCount == 4)
        {
            key_Ctrl_Alt_Right();
        }
    }
}

QPoint MultiFingerGesture::pointCalibProcess(int x, int y)
{
    int x1 = (int)((float)x / (float)32767 * m_sw);
    int y1 = (int)((float)y / (float)32767 * m_sh);
    return QPoint(x1,y1);
}

int MultiFingerGesture::calculateTwoPointDistance(int x1, int y1, int x2, int y2)
{
    int distance = 0;
    int deltaXTemp = x2 - x1;
    int deltaYTemp = y2 - y1;
    distance = sqrt(pow((double)deltaXTemp,2.0) + pow((double)deltaYTemp,2));
    return distance;
}

void MultiFingerGesture::mouseDown(int button)
{
    XTestFakeButtonEvent(display,button,true,0);
    XFlush(display);

}

void MultiFingerGesture::mouseUp(int button)
{
    XTestFakeButtonEvent(display,button,false,0);
    XFlush(display);
}

void MultiFingerGesture::mouseMove(int x, int y)
{

    XTestFakeMotionEvent(display,DefaultScreen(display),x,y,0);
    XFlush(display);
}

void MultiFingerGesture::mouseClick(int button)
{
    this->mouseDown(button);
    usleep(100);
    this->mouseUp(button);

}

void MultiFingerGesture::mouseDrag(int x, int y)
{
    this->mouseMove(x,y);
}

void MultiFingerGesture::keyDown(int key)
{
    XTestFakeKeyEvent(display,key,true,0);
    XFlush(display);
}

void MultiFingerGesture::keyUp(int key)
{
    XTestFakeKeyEvent(display,key,false,0);
    XFlush(display);
}

void MultiFingerGesture::keyClick(int key)
{
    this->keyDown(key);
    this->keyUp(key);
    XFlush(display);

}

void MultiFingerGesture::key_Shift_Ctrl_R()
{
    int shift_keycode = XKeysymToKeycode(display,XStringToKeysym("Shift_L"));
    int ctrl_keycode = XKeysymToKeycode(display,XStringToKeysym("Control_L"));
    int r_keycode = XKeysymToKeycode(display,XStringToKeysym("r"));

    keyDown(shift_keycode);
    keyDown(ctrl_keycode);
    keyDown(r_keycode);

    ctrlKeyDownFlag = true;
    shiftKeyDownFlag = true;

    keyUp(r_keycode);
    keyUp(shift_keycode);
    keyUp(ctrl_keycode);

    ctrlKeyDownFlag = false;
    shiftKeyDownFlag = false;
}

void MultiFingerGesture::key_Ctrl_R()
{
    int ctrl_keycode = XKeysymToKeycode(display,XStringToKeysym("Control_L"));
    int r_keycode = XKeysymToKeycode(display,XStringToKeysym("r"));

    keyDown(ctrl_keycode);
    keyDown(r_keycode);
    ctrlKeyDownFlag = true;

    keyUp(r_keycode);
    keyUp(ctrl_keycode);

    ctrlKeyDownFlag = false;

}

void MultiFingerGesture::key_Ctrl_Minus()
{

    int ctrl_keycode = XKeysymToKeycode(display,XStringToKeysym("Control_L"));
    //  int minus_keycode = XKeysymToKeycode(display,XStringToKeysym("minus"));
    int minus_keycode = XKeysymToKeycode(display,XStringToKeysym("KP_Subtract"));

    keyDown(ctrl_keycode);
    keyDown(minus_keycode);
    ctrlKeyDownFlag = true;

    keyUp(minus_keycode);
    keyUp(ctrl_keycode);
    ctrlKeyDownFlag = false;
}

void MultiFingerGesture::key_Ctrl_Plus()
{

    int ctrl_keycode = XKeysymToKeycode(display,XStringToKeysym("Control_L"));
    //  int plus_keycode = XKeysymToKeycode(display,XStringToKeysym("plus"));
    int plus_keycode = XKeysymToKeycode(display,XStringToKeysym("KP_Add"));

    keyDown(ctrl_keycode);
    keyDown(plus_keycode);
    ctrlKeyDownFlag = true;

    keyUp(plus_keycode);
    keyUp(ctrl_keycode);
    ctrlKeyDownFlag = false;

}

void MultiFingerGesture::key_Alt_Left()
{
    int alt_keycode = XKeysymToKeycode(display,XStringToKeysym("Meta_L"));
    int left_keycode = XKeysymToKeycode(display,XStringToKeysym("Left"));

    keyDown(alt_keycode);
    keyDown(left_keycode);

    keyUp(left_keycode);
    keyUp(alt_keycode);
}

void MultiFingerGesture::key_Alt_Right()
{
    int alt_keycode = XKeysymToKeycode(display,XStringToKeysym("Meta_L"));
    int right_keycode = XKeysymToKeycode(display,XStringToKeysym("Right"));

    keyDown(alt_keycode);
    keyDown(right_keycode);

    keyUp(right_keycode);
    keyUp(alt_keycode);
}

void MultiFingerGesture::key_Ctrl_Alt_D()
{
    gestureDone = true;
    int ctrl_keycode = XKeysymToKeycode(display,XStringToKeysym("Control_L"));
    int alt_keycode = XKeysymToKeycode(display,XStringToKeysym("Meta_L"));
    int d_keycode = XKeysymToKeycode(display,XStringToKeysym("d"));

    keyDown(ctrl_keycode);
    keyDown(alt_keycode);
    keyDown(d_keycode);

    ctrlKeyDownFlag = true;
    keyUp(d_keycode);
    keyUp(ctrl_keycode);
    keyUp(alt_keycode);

    ctrlKeyDownFlag = false;
}

void MultiFingerGesture::key_Super_D()
{
    gestureDone = true;
    // super
    int super_keycode = XKeysymToKeycode(display,XStringToKeysym("Super_L"));
    int d_keycode = XKeysymToKeycode(display,XStringToKeysym("d"));

    keyDown(super_keycode);
    keyDown(d_keycode);

    keyUp(d_keycode);
    keyUp(super_keycode);
}

void MultiFingerGesture::key_Super()
{
    gestureDone = true;
    // super
    int super_keycode = XKeysymToKeycode(display,XStringToKeysym("Super_L"));

    keyDown(super_keycode);
    keyUp(super_keycode);
}

void MultiFingerGesture::key_Ctrl_Alt_Up()
{
    gestureDone = true;
    // ctrl alt up

    int ctrl_keycode = XKeysymToKeycode(display,XStringToKeysym("Control_L"));
    int alt_keycode = XKeysymToKeycode(display,XStringToKeysym("Meta_L"));
    int up_keycode = XKeysymToKeycode(display,XStringToKeysym("Up"));

    keyDown(ctrl_keycode);
    keyDown(alt_keycode);
    keyDown(up_keycode);

    ctrlKeyDownFlag = true;

    keyUp(up_keycode);
    keyUp(ctrl_keycode);
    keyUp(alt_keycode);

    ctrlKeyDownFlag = false;
}

void MultiFingerGesture::key_Ctrl_Alt_Down()
{
    gestureDone = true;
    // ctrl alt down


    int ctrl_keycode = XKeysymToKeycode(display,XStringToKeysym("Control_L"));
    int alt_keycode = XKeysymToKeycode(display,XStringToKeysym("Meta_L"));
    int down_keycode = XKeysymToKeycode(display,XStringToKeysym("Down"));

    keyDown(ctrl_keycode);
    keyDown(alt_keycode);
    keyDown(down_keycode);

    ctrlKeyDownFlag = true;

    keyUp(down_keycode);
    keyUp(ctrl_keycode);
    keyUp(alt_keycode);

    ctrlKeyDownFlag = false;
}

void MultiFingerGesture::key_Ctrl_Alt_Left()
{
    gestureDone = true;
    // ctrl alt left


    int ctrl_keycode = XKeysymToKeycode(display,XStringToKeysym("Control_L"));
    int alt_keycode = XKeysymToKeycode(display,XStringToKeysym("Meta_L"));
    int left_keycode = XKeysymToKeycode(display,XStringToKeysym("Left"));

    keyDown(ctrl_keycode);
    keyDown(alt_keycode);
    keyDown(left_keycode);

    ctrlKeyDownFlag = true;

    keyUp(left_keycode);
    keyUp(ctrl_keycode);
    keyUp(alt_keycode);

    ctrlKeyDownFlag = false;
}

void MultiFingerGesture::key_Ctrl_Alt_Right()
{
    gestureDone = true;
    // ctrl alt right


    int ctrl_keycode = XKeysymToKeycode(display,XStringToKeysym("Control_L"));
    int alt_keycode = XKeysymToKeycode(display,XStringToKeysym("Meta_L"));
    int right_keycode = XKeysymToKeycode(display,XStringToKeysym("Right"));

    keyDown(ctrl_keycode);
    keyDown(alt_keycode);
    keyDown(right_keycode);

    ctrlKeyDownFlag = true;

    keyUp(right_keycode);
    keyUp(ctrl_keycode);
    keyUp(alt_keycode);

    ctrlKeyDownFlag = false;
}


MultiFingerGesture::~MultiFingerGesture()
{
    XCloseDisplay(display);
}
