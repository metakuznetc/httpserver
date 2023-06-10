#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "qurlquery.h"
#include <QTcpServer>

class HttpServer : public QTcpServer {
    Q_OBJECT
public:
    explicit HttpServer(QObject* parent = nullptr);
protected:
    void incomingConnection(qintptr socketDescriptor) override;
    void processRequest(QTcpSocket* socket, const QByteArray& requestData);

private:
    void handleGetSumRequest(QTcpSocket* socket, const QUrlQuery& query);
    void handleManagersRequest(QTcpSocket* socket, const QUrlQuery& query);
    void handleNameChange();
};

#endif // HTTPSERVER_H
