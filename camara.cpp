#include "camara.h"
#include "ui_camara.h"
#include <QTextStream>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QStringList>
#include <QTime>

Camara::Camara(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Camara)
{
    ui->setupUi(this);
    this->setFixedSize(395, 300);
    this->ui->connectButton->setEnabled(false);
    this->ui->stopButton->setEnabled(false);
    this->stopped = false;
    this->tcpSocket = NULL;

    QMessageBox::information(this, "Info", "Please specify camara file");
    camaraFileName = QFileDialog::getOpenFileName(this, "Open Camara File", "D:/Qt/project/tracking_system/resources");
    QMessageBox::information(this, "Info", "Please specify data file");
    dataFileName = QFileDialog::getOpenFileName(this, "Open Data File", "D:/Qt/project/tracking_system/resources");

    QFile sensorFile(camaraFileName);
    if(sensorFile.open(QIODevice::ReadOnly))
    {
        QTextStream sensorIn(&sensorFile);
        while(!sensorIn.atEnd())
        {
            QString line = sensorIn.readLine();
            QStringList list = line.split(",");
            MySensor tempSensor;
            tempSensor.x = list[0].toInt();
            tempSensor.y = list[1].toInt();
            tempSensor.z = list[2].toInt();
            tempSensor.xc = list[0].toInt();
            tempSensor.yc = list[1].toInt();
            tempSensor.zc = list[2].toInt();
            tempSensor.alpha = list[3].toInt();
            tempSensor.beta = list[4].toInt();
            tempSensor.zeta = list[5].toInt();
            tempSensor.zoom = 0;
            direction(tempSensor);
            sensorList.append(tempSensor);
        }
        ui->textBrowser->append(QString("%1 Camara positions have been set\n").arg(sensorList.size()));
    }
    else
    {
        QMessageBox::warning(this, tr("Error"), tr("unable to open sensor info file"));
        return;
    }
}

Camara::~Camara()
{
    delete ui;
}

void Camara::on_connectButton_clicked()
{
    stopped = false;
    tcpSocket = new QTcpSocket();
    // connecting
    tcpSocket->connectToHost(ui->lineEdit->text(), 1207);
    bool isConnected = tcpSocket->waitForConnected();
    if(!isConnected)
    {
        ui->textBrowser->append("Server connection error!\n");
    }
    else
    {
        ui->textBrowser->append("Server Connected\n");
        ui->connectButton->setEnabled(false);
        ui->stopButton->setEnabled(true);
        connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(scktReadData()));
        times = 0;
        while(!stopped)
        {
            //random generate coordinates for objects

            data.clear();
            points.clear();
            //waiting for 1 second
            QTime t;
            t.start();
            times = this->genObjCoor(times);
            if (times == -1) exit(1);
            while(t.elapsed()<1000)
            {
                QCoreApplication::processEvents();
            }

            // read new data
            QFile dataFile(dataFileName);
            QDataStream in(&dataFile);
            if(!dataFile.open(QIODevice::ReadOnly))
            {
                ui->textBrowser->append("Read data file error\n");
                return;
            }
            while(in.atEnd() != true)
            {
                int num, x, y, z;
                in >> num >> x >> y >> z;
                QList<int> line;
                line.append(num);
                line.append(x);
                line.append(y);
                line.append(z);
                points.append(line);
            }
            // process new data
            for(int j = 0; j < sensorList.size(); j++)
            {
                for(int i = 0; i < points.size(); i++)
                {
                    double screenx, screeny;
                    int a, b, c;
                    a = points[i][1];
                    b = points[i][2];
                    c = points[i][3];
                    transform(sensorList[j], a, b, c, screenx, screeny);
                    if(screenx != -1)
                    {
                        data += QString("%1,%2,%3,%4,%5 ").arg(j).arg(points[i][0]).arg(screenx).arg(screeny).arg(points[i][3]);
                    }
                }
            }
            for(int i = 0; i < points.size(); i++)
            {
                data += QString("%1,%2,%3,%4,%5 ").arg(4).arg(points[i][0]).arg(points[i][1]).arg(points[i][2]).arg(points[i][3]);
            }
            data.chop(1);


            // write data to server
            tcpSocket->write(data.toStdString().c_str(), strlen(data.toStdString().c_str()));
            tcpSocket->waitForBytesWritten();
            ui->textBrowser->append(data + QString(" sent!\n"));

            /*//read data from server
            t.restart();
            while(t.elapsed()<2000)
            {
                QCoreApplication::processEvents();
            }

            QString newData(tcpSocket->readAll());
            ui->textBrowser->append(newData + QString("received!\n"));
            QStringList list = newData.split(" ");
            for(int i = 0; i < list.size(); i++)
            {
                QStringList parameters = list[i].split(",");
                int camNum = parameters[0].toInt();
                double alpha = parameters[1].toDouble();
                double beta = parameters[2].toDouble();
                double zoom = parameters[3].toDouble();
                updateCPos(camNum, alpha, beta, zoom);
            }*/
        }
        tcpSocket->disconnectFromHost();
        tcpSocket->waitForDisconnected();
        ui->textBrowser->append("Connection stopped");
        delete tcpSocket;
    }
}

void Camara::on_lineEdit_textChanged(const QString &arg1)
{
    QRegExp rx2("^(25[0-5]|2[0-4][0-9]|[0-1]{1}[0-9]{2}|[1-9]{1}[0-9]{1}|[1-9])\.(25[0-5]|2[0-4][0-9]|[0-1]{1}[0-9]{2}|[1-9]{1}[0-9]{1}|[1-9]|0)\.(25[0-5]|2[0-4][0-9]|[0-1]{1}[0-9]{2}|[1-9]{1}[0-9]{1}|[1-9]|0)\.(25[0-5]|2[0-4][0-9]|[0-1]{1}[0-9]{2}|[1-9]{1}[0-9]{1}|[0-9])$");
    if(rx2.exactMatch(arg1))
    {
        ui->connectButton->setEnabled(true);
    }
    else
    {
        ui->connectButton->setEnabled(false);
    }
}

void Camara::on_stopButton_clicked()
{
    this->stopped = true;
    ui->stopButton->setEnabled(false);
    ui->connectButton->setEnabled(true);
}

void Camara::transform(MySensor sensor, int a, int b, int c, double &screen_x, double &screen_y)
{
    double x, y, z, alpha, beta, zeta;
    x = sensor.xc;
    y = sensor.yc;
    z = sensor.zc;
    alpha = sensor.alpha;
    beta = sensor.beta;
    zeta = sensor.zeta;
    double alpha_c, beta_c;
    double m, n;

    if (a - x < 0)
        alpha_c = 180 - atan((b - y) / (a - x)) * 180 / PI;
        //alpha_c = atan2(b - y, a - x) * 180 / PI + 360;
    else
        alpha_c = - atan((b - y) / (a - x)) * 180 / PI;
        //alpha_c = atan2(b - y, a - x) * 180 / PI;
    if (alpha_c < 0)
    {
        alpha_c += 360;
    }

    beta_c = atan((c - z) /
                   sqrt((a - x) * (a - x) + (b - y)* (b - y))) * 180 / PI;
    //beta_c = atan((c-z) / (b - y)) * 180 / PI;
    //beta_c = atan2(c-z, b - y) * 180 / PI;

    m = tan((alpha_c - alpha) * PI / 180 ) / tan((zeta) * PI / 180 );
    n = tan((beta_c - beta) * PI / 180 ) / tan((zeta) * PI / 180 );

    //cout << alpha_c << " " << beta_c << " " << m << " " << n ;

    screen_x = - m * 0.5 * WIDTH + 0.5 * WIDTH;
    screen_y = - n * 0.5 * HEIGHT + 0.5 *HEIGHT;
    screen_x = (int)(screen_x + 0.5);
    screen_y = (int)(screen_y + 0.5);

    if (screen_x > 100 || screen_x < 0 || screen_y > 100 || screen_y < 0)
    {
        screen_x = -1;
        screen_y = -1;
    }
    /*if ((abs(alpha_c - alpha) > zeta) || (abs(beta_c - beta) > zeta))
    {
        screen_x = -1;
        screen_y = -1;
    }*/
}

void Camara::direction(MySensor &sensor)
{
    sensor.x_dir = cos(sensor.alpha * PI / 180);
    sensor.y_dir = sin(sensor.alpha * PI / 180);
    sensor.z_dir = tan(sensor.zeta * PI / 180);
}

void Camara::scktReadData()
{
    QString newData(tcpSocket->readAll());
    ui->textBrowser->append(newData + QString("received!\n"));
    QStringList list = newData.split(" ");
    for(int i = 0; i < list.size(); i++)
    {
        QStringList parameters = list[i].split(",");
        int camNum = parameters[0].toInt();
        double alpha = parameters[1].toDouble();
        double beta = parameters[2].toDouble();
        double zoom = parameters[3].toDouble();
        updateCPos(camNum, alpha, beta, zoom);
    }
}

void Camara::updateCPos(int camNum, double alpha, double beta, double zoom)
{
    sensorList[camNum].alpha += alpha;
    if(sensorList[camNum].alpha > 360)
    {
        sensorList[camNum].alpha -= 360;
    }
    else if(sensorList[camNum].alpha < 0)
    {
        sensorList[camNum].alpha += 360;
    }
    sensorList[camNum].beta += beta;
    if(sensorList[camNum].beta > 360)
    {
        sensorList[camNum].beta -= 360;
    }
    else if(sensorList[camNum].beta < 0)
    {
        sensorList[camNum].beta += 360;
    }
    sensorList[camNum].zoom += zoom;
    ui->textBrowser->append(QString("cam:%1, alpha:%2, beta:%3, zoom:%4").arg(camNum).arg(alpha).arg(beta).arg(zoom));
}

void Camara::on_action_Quit_triggered()
{
    qApp->quit();
}

void Camara::on_action_Open_triggered()
{
    QMessageBox::information(this, "Info", "Please specify camara file");
    camaraFileName = QFileDialog::getOpenFileName(this, "Open Camara File", "D:/Qt/project/tracking_system/resources");
    QMessageBox::information(this, "Info", "Please specify data file");
    dataFileName = QFileDialog::getOpenFileName(this, "Open Data File", "D:/Qt/project/tracking_system/resources");

    QFile sensorFile(camaraFileName);
    if(sensorFile.open(QIODevice::ReadOnly))
    {
        QTextStream sensorIn(&sensorFile);
        while(!sensorIn.atEnd())
        {
            QString line = sensorIn.readLine();
            QStringList list = line.split(",");
            MySensor tempSensor;
            tempSensor.x = list[0].toInt();
            tempSensor.y = list[1].toInt();
            tempSensor.z = list[2].toInt();
            tempSensor.xc = list[0].toInt();
            tempSensor.yc = list[1].toInt();
            tempSensor.zc = list[2].toInt();
            tempSensor.alpha = list[3].toInt();
            tempSensor.beta = list[4].toInt();
            tempSensor.zeta = list[5].toInt();
            tempSensor.zoom = 0;
            direction(tempSensor);
            sensorList.append(tempSensor);
        }
        ui->textBrowser->append(QString("%1 Camara positions have been set\n").arg(sensorList.size()));
    }
}

void Camara::changeCameraAngle(QStringList list, QString cameraFileName)
{
    QFile cameraFile(cameraFileName);
    if(cameraFile.open(QIODevice::ReadWrite))
    {
        QTextStream in(&cameraFile);
        QString allInfo = in.readAll();
        QStringList sepaCamera = allInfo.split("\r\n");
        QStringList sepaAngle = sepaCamera[list[0].toInt()].split(",");
        sepaAngle[3] = QString::number(list[1].toInt());
        sepaAngle[4] = QString::number(list[2].toInt());

        QTextStream out(&cameraFile);
        out << allInfo;
    }
    cameraFile.close();
}

int Camara::genObjCoor(int times)
{
    //not first time, read data from previous iteration
    if (times !=0)
    {
        QFile file(dataFileName);
        if (!file.open(QIODevice::ReadOnly))
        {
            std::cerr << "Cannot open file for reading:"
                      << qPrintable(file.errorString()) << std::endl;
            return times = -1;
        }

        QDataStream in(&file);
        objNum = 0;
        for (int i = 0; i < 8; ++i)
        {
            in >> object[i].objSeq >> object[i].objx >> object[i].objy >> object[i].objz;
            if (object[i].objSeq != 0)
            {
                objNum++;
            }
        }
        file.close();

        for (int i = objNum; i <= 7; ++i)
        {
            object[i].objSeq = -1;
            object[i].objx = -1;
            object[i].objy = -1;
            object[i].objz = -1;
        }
    }

    //first time, randomly generate the starting coordinates
    if (times == 0)
    {
        //initialization of objects including sequence number, x, y, z
        for (int i = 0; i <= 7; ++i)
        {
            object[i].objSeq = -1;
            object[i].objx = -1;
            object[i].objy = -1;
            object[i].objz = -1;
        }

        objNum = 8;
        newObjSeq = objNum;

        for (int i = 0; i < objNum; ++i)
        {
            object[i].objSeq = i + 1; //sequence number starts from 1
            object[i].objx = (qrand() % 1000); //space range 0 to 1000
            object[i].objy = (qrand() % 1000);
            object[i].objz = (qrand() % 10); //height range 0 to 10
        }

        //write into dat file
        QFile file(dataFileName);
        file.resize(0);
        if (!file.open(QIODevice::Append))
        {
            std::cerr << "Cannot open file for writing:"
                      << qPrintable(file.errorString()) << std::endl;
            return times = -1;
        }

        QDataStream out(&file);
        //out.setVersion(QDataStream::Qt_4_3);
        for (int i = 0; i < objNum; ++i)
        {
            out << object[i].objSeq << object[i].objx << object[i].objy << object[i].objz;
        }
        file.close();

    }

    //not first time, move objects
    else
    {
        //move objects
        for (int i = 0; i < objNum; ++i)
        {
            stepx = (qrand() % 11) - 5; //move speed from -15 to 15
            stepy = (qrand() % 11) - 5;
            //choose = (qrand() % 2);

            object[i].objx = object[i].objx + stepx;
            object[i].objy = object[i].objy + stepy;

            /*if (choose == 0)
            {
                object[i].objx = object[i].objx + stepx;
            }

            else
            {
                object[i].objy = object[i].objy + stepy;
            }*/

            if (object[i].objx < 0 || object[i].objx > 1000 || object[i].objy < 0 || object[i].objy > 1000)
            {
                object[i].objSeq = -2;
            }
        }

        //when objects have moved out of range, delete corresponding information
        int minus = 0;
        for (int i = 7; i >= 0; --i)
        {
            if (object[i].objSeq != -1)
            {
                if (object[i].objSeq == -2 && i != 7)
                {
                    for (int j = i; j <= 7; ++j)
                    {
                        object[j] = object[j + 1];
                    }
                    object[7].objSeq = -1;
                    object[7].objx = -1;
                    object[7].objy = -1;
                    object[7].objz = -1;
                    minus++;
                }

                else if (object[i].objSeq == -2 && i == 7)
                {
                    object[7].objSeq = -1;
                    object[7].objx = -1;
                    object[7].objy = -1;
                    object[7].objz = -1;
                    minus++;
                }
            }
        }
        objNum = objNum - minus;

        //when new objects appear in the range, create new sequence number and add corresponding information to the end of object[100]
        int plus = (qrand() % (9 - objNum));
        newObjSeq = object[objNum - 1].objSeq + plus;

        for (int i = objNum + plus - 1; i >= objNum; --i)
        {
            object[i].objSeq = newObjSeq;
            object[i].objx = (qrand() % 1000);
            object[i].objy = (qrand() % 1000);
            object[i].objz = (qrand() % 100);
            newObjSeq--;
        }
        objNum = objNum + plus;
        newObjSeq = newObjSeq + plus;

        //write into dat file
        QFile file(dataFileName);
        file.resize(0);
        if (!file.open(QIODevice::WriteOnly))
        {
            std::cerr << "Cannot open file for writing:"
                      << qPrintable(file.errorString()) << std::endl;
            return times = -1;
        }

        QDataStream out(&file);
        //out.setVersion(QDataStream::Qt_4_3);
        for (int i = 0; i < objNum; ++i)
        {
            out << object[i].objSeq << object[i].objx << object[i].objy << object[i].objz;
        }
        file.close();

    }

    times++;
    return times;
}
