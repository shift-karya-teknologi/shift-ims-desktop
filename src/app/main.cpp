#include <QApplication>
#include <QSharedMemory>
#include <QBuffer>
#include <QFile>
#include <QDebug>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>

#include "global.h"
#include "mainwindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    app.setApplicationName(SIMS_APP_NAME);
    app.setApplicationDisplayName(SIMS_APP_DISPLAY_NAME);
    app.setApplicationVersion(SIMS_VERSION_STR);

    // Simple single instance application mechanism with shared memory
    QSharedMemory appPidSharedMemory("shift-ims-desktop.pid");
    if (appPidSharedMemory.attach()) {
        QBuffer buffer;
        qint64 pid;
        QDataStream in(&buffer);

        appPidSharedMemory.lock();
        buffer.setData((char*)appPidSharedMemory.constData(), appPidSharedMemory.size());
        buffer.open(QBuffer::ReadOnly);
        in >> pid;
        appPidSharedMemory.unlock();

        qWarning() << "Shift IMS Desktop is already running with PID" << pid;
        return 0;
    }
    else {
        QBuffer buffer;
        buffer.open(QBuffer::WriteOnly);

        QDataStream out(&buffer);
        out << app.applicationPid();

        if (!appPidSharedMemory.create(buffer.size())) {
            qCritical() << "Unable to create shared memory segment. Error:" << appPidSharedMemory.errorString();
            return 1;
        }

        appPidSharedMemory.lock();
        char *to = (char*)appPidSharedMemory.data();
        const char *from = buffer.data().data();
        memcpy(to, from, qMin(appPidSharedMemory.size(), (int)buffer.size()));
        appPidSharedMemory.unlock();
    }

    {
        QSettings settings(SIMS_DEFAULT_SETTINGS_PATH, QSettings::IniFormat);
        QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");

        db.setHostName(settings.value("Database/hostname").toString());
        db.setPort(settings.value("Database/port").toInt());
        db.setDatabaseName(settings.value("Database/schema").toString());
        db.setUserName(settings.value("Database/username").toString());
        db.setPassword(settings.value("Database/password").toString());

        if (!db.open()) {
            qCritical() << "Database connection failed:" << qPrintable(db.lastError().text());
            return 2;
        }
    }

    MainWindow mw;
    mw.showMaximized();

    return app.exec();
}
