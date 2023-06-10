#include "httpserver.h"
#include <QTcpSocket>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSqlQuery>
#include <QTextStream>
#include <QSqlDriver>
#include <QSqlError>

HttpServer::HttpServer(QObject* parent)
    : QTcpServer(parent) {

    QSqlDatabase db = QSqlDatabase::database("qt_sql_default_connection");
    if (db.isValid()) {
        db.close();
    }

    db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("127.0.0.1");
    db.setPort(2461);
    db.setDatabaseName("postgres");
    db.setUserName("user1");
    db.setPassword("1p2a3s4s");

    if (!db.open()) {
        QJsonObject errorResponse;
        errorResponse["error"] = "Failed to connect to the database.";
        QJsonDocument jsonResponse(errorResponse);
        QByteArray responseData = jsonResponse.toJson();

        return;
    }

    QSqlQuery triggerQuery(db);
    QString triggerString = "CREATE TRIGGER IF NOT EXISTS name_update_trigger "
                            "AFTER UPDATE OF name ON managers "
                            "FOR EACH ROW "
                            "EXECUTE FUNCTION name_update_function();";
    if (triggerQuery.exec(triggerString)) {
        qDebug() << "Успешно создан триггер на таблице 'managers' для изменений поля 'name'";
    } else {
        qDebug() << "Ошибка при создании триггера на таблице 'managers':";
        qDebug() << triggerQuery.lastError().text();
    }
}

void HttpServer::incomingConnection(qintptr socketDescriptor) {
    QTcpSocket* socket = new QTcpSocket(this);

    if (!socket->setSocketDescriptor(socketDescriptor)) {
        socket->deleteLater();
        return;
    }

    socket->setReadBufferSize(4096);

    connect(socket, &QTcpSocket::readyRead, this, [=]() {
        QByteArray requestData = socket->readAll();
        processRequest(socket, requestData);
    });

    connect(socket, &QTcpSocket::disconnected, this, [=]() {
        socket->deleteLater();
    });
}

void HttpServer::processRequest(QTcpSocket* socket, const QByteArray& requestData) {
    QString requestString = QString::fromUtf8(requestData);
    QStringList requestLines = requestString.split('\n');
    QString requestMethod = requestLines.first().split(' ').first();
    QString requestUrl = requestLines.first().split(' ').at(1);

    if (requestMethod == "GET" && requestUrl.startsWith("/api/v1/get_sum")) {
        QUrl url(requestUrl);
        QUrlQuery query(url.query());
        int a = query.queryItemValue("a").toInt();
        int b = query.queryItemValue("b").toInt();
        int sum = a + b;

        QJsonObject response;
        response["a"] = a;
        response["b"] = b;
        response["sum"] = sum;

        QJsonDocument jsonResponse(response);
        QByteArray responseData = jsonResponse.toJson();

        socket->write("HTTP/1.1 200 OK\r\n");
        socket->write("Content-Type: application/json\r\n");
        socket->write("Content-Length: " + QByteArray::number(responseData.size()) + "\r\n");
        socket->write("\r\n");
        socket->write(responseData);
    }
    else if (requestMethod == "GET" && requestUrl.startsWith("/api/v1/managers")) {
        QUrl url(requestUrl);
        QUrlQuery query(url.query());
        QString name = query.queryItemValue("name");

        QSqlQuery dbQuery;
        QString queryString = "SELECT * FROM managers WHERE name = ?";
        dbQuery.prepare(queryString);
        dbQuery.bindValue(0, name);
        if (dbQuery.exec()) {
            QJsonArray managersArray;
            while (dbQuery.next()) {
                QJsonObject manager;
                manager["id"] = dbQuery.value("id").toInt();
                manager["name"] = dbQuery.value("name").toString();
                manager["family"] = dbQuery.value("family").toString();
                managersArray.append(manager);
            }

            QJsonDocument jsonResponse(managersArray);
            QByteArray responseData = jsonResponse.toJson();

            socket->write("HTTP/1.1 200 OK\r\n");
            socket->write("Content-Type: application/json\r\n");
            socket->write("Content-Length: " + QByteArray::number(responseData.size()) + "\r\n");
            socket->write("\r\n");
            socket->write(responseData);
        }
        else {
            QJsonObject errorResponse;
            errorResponse["error"] = "Failed to retrieve managers from the database.";
            QJsonDocument jsonResponse(errorResponse);
            QByteArray responseData = jsonResponse.toJson();

            socket->write("HTTP/1.1 500 Internal Server Error\r\n");
            socket->write("Content-Type: application/json\r\n");
            socket->write("Content-Length: " + QByteArray::number(responseData.size()) + "\r\n");
            socket->write("\r\n");
            socket->write(responseData);
        }


    }
    else {
        // Обработка неподдерживаемого запроса
        // ...
    }
}
