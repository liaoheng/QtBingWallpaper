/****************************************************************************
**
** Copyright (C) 2018 liaoheng.
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Solutions component.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QThread>
#include <QtNetwork>

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

#include "qtservice.h"
#include "json.h"

using QtJson::JsonObject;
using QtJson::JsonArray;

using namespace std;

class WallPaperDaemonThread : public QThread
{
    Q_OBJECT
public:
    explicit WallPaperDaemonThread(QObject *parent = 0)
        : QThread(parent)
    {
        qDebug() << "WallPaperDaemonThread create: " << QThread::currentThreadId();
    }
    ~WallPaperDaemonThread()
    {
        requestInterruption();
        quit();
        wait();
        if(currentDownload!=nullptr)
            delete currentDownload;
    }
private:
    QFile output;
    QNetworkReply *currentDownload = nullptr;
private slots:
    void downloadReadyRead()
    {
        output.write(currentDownload->readAll());
    }
protected:
    void run() Q_DECL_OVERRIDE {

        QString url = "http://cn.bing.com/HPImageArchive.aspx?format=js&idx=0&n=1";

        QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
        QNetworkRequest networkRequest;
        QNetworkReply* reply;
        QEventLoop connection_loop;
        connect(networkManager, SIGNAL( finished( QNetworkReply* ) ), &connection_loop, SLOT( quit() ) );
        networkRequest.setUrl( url );
        qDebug() << "reaching url: " << url;
        reply = networkManager->get( networkRequest );
        connection_loop.exec();
        qDebug() << "get -> " << url << ", done, size: " << reply->bytesAvailable();
        reply->deleteLater();

        QByteArray json =reply->readAll();

        delete reply;

        JsonObject result = QtJson::parse(json).toMap();
        QList<QVariant> images=result["images"].toList();

        JsonObject image =images[0].toMap();
        QString imageUrl=image["url"].toString();
        imageUrl="http://cn.bing.com"+imageUrl;

        qDebug()<< imageUrl;

        QString filename = QFileInfo(imageUrl).fileName();

        if (filename.isEmpty())
            filename = "download";
        QString path=nullptr;
#if defined(Q_OS_WIN)
        path= "d:/"+filename;
#endif
        if(path==nullptr){
            return;
        }
        output.setFileName(path);

        output.open(QIODevice::WriteOnly);

        QNetworkRequest d_networkRequest;

        QEventLoop d_connection_loop;
        connect(networkManager, SIGNAL( finished( QNetworkReply* ) ), &d_connection_loop, SLOT( quit() ) );
        d_networkRequest.setUrl( imageUrl );
        currentDownload = networkManager->get( d_networkRequest );
        d_connection_loop.exec();
        qDebug() << "get -> " << imageUrl << ", done, size: " << currentDownload->bytesAvailable();
        output.write(currentDownload->readAll());
        output.close();

        currentDownload->deleteLater();
        qDebug()<< "ok path "<<path;

        msleep(1000);

#if defined(Q_OS_WIN)
        SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (PVOID)path.utf16(), SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
#endif
    }


};


class HttpService : public QtService<QCoreApplication>
{
public:
    HttpService(int argc, char **argv)
        : QtService<QCoreApplication>(argc, argv, "Qt HTTP Daemon")
    {
        setServiceDescription("A dummy HTTP service implemented with Qt");
        setServiceFlags(QtServiceBase::CanBeSuspended);
    }
private:
    WallPaperDaemonThread *daemon;

protected:
    virtual void start() Q_DECL_OVERRIDE
    {
        daemon = new WallPaperDaemonThread;
        daemon->start();
    }

    void stop() Q_DECL_OVERRIDE
    {
        daemon->quit();
    }
};

#include "bingwallpaperservice.moc"

int main(int argc, char **argv)
{
#if !defined(Q_OS_WIN)
    // QtService stores service settings in SystemScope, which normally require root privileges.
    // To allow testing this example as non-root, we change the directory of the SystemScope settings file.
    QSettings::setPath(QSettings::NativeFormat, QSettings::SystemScope, QDir::tempPath());
    qWarning("(Example uses dummy settings file: %s/QtSoftware.conf)", QDir::tempPath().toLatin1().constData());
#endif
    HttpService service(argc, argv);
    return service.exec();
}
