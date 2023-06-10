#include <QCoreApplication>
#include "httpserver.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);


    // Создание экземпляра сервера
    HttpServer server;

    // Попытка запуска сервера на порту 8080
    if (!server.listen(QHostAddress::Any, 8080)) {
        qDebug() << "Failed to start server.";
        return -1;
    }
    else qDebug() << "Server started on port 8080.";

    return a.exec();
}
