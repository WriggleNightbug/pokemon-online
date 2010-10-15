#include "network.h"
#include "antidos.h"

Network::Network(QTcpSocket *sock, int id) : mysocket(sock), commandStarted(false), myid(id), stillValid(true)
{
    connect(socket(), SIGNAL(readyRead()), this, SLOT(onReceipt()));
    connect(socket(), SIGNAL(disconnected()), this, SIGNAL(disconnected()));
    connect(socket(), SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(manageError(QAbstractSocket::SocketError)));
    /* SO THE SOCKET IS SAFELY DELETED LATER WHEN DISCONNECTED! */
    connect(socket(), SIGNAL(disconnected()), socket(), SLOT(deleteLater()));
    connect(this, SIGNAL(destroyed()), socket(), SLOT(deleteLater()));
    _ip = socket()->peerAddress().toString();
}

void Network::manageError(QAbstractSocket::SocketError err)
{
    myerror = err;
}

void Network::close() {
    stillValid = false;
    if (socket()) {
        QTcpSocket *sock = mysocket;
        mysocket = NULL;
        sock->disconnect();
        sock->disconnectFromHost();
        sock->deleteLater();
        emit disconnected();
    }
}

void Network::setLowDelay(bool lowDelay)
{
    if (socket()) {
        socket()->setSocketOption(QAbstractSocket::LowDelayOption, lowDelay);
    }
}

Network::~Network()
{
    close();
}

void Network::connectToHost(const QString &ip, quint16 port)
{
    socket()->connectToHost(ip, port);
    connect(socket(), SIGNAL(connected()), SIGNAL(connected()));
}

bool Network::isConnected() const
{
    if (socket())
        return socket()->state() != QAbstractSocket::UnconnectedState;
    else
        return false;
}

QString Network::ip() const {
    return _ip;
}

void Network::onDisconnect()
{
    mysocket = NULL;
}

void Network::onReceipt()
{
    if (stillValid && socket())
    {
        if (commandStarted == false) {
            /* There it's a new message we are receiving.
               To start receiving it we must know its length, i.e. the 2 first bytes */
            if (socket()->bytesAvailable() < 2) {
                return;
            }
            /* Ok now we can start */
            commandStarted=true;
            /* getting the length of the message */
            char c1, c2;
            socket()->getChar(&c1), socket()->getChar(&c2);
            remainingLength=uchar(c1)*256+uchar(c2);

            /* Just a little check :p */
            if (!AntiDos::obj()->transferBegin(myid, remainingLength, ip())) {
                return;
            }

            /* Recursive call to write less code =) */
            onReceipt();
        } else {
            /* Checking if the command is complete! */
            if (socket()->bytesAvailable() >= remainingLength) {
                emit isFull(socket()->read(remainingLength));
                commandStarted = false;
                /* Recursive call to spare code =), there may be still data pending */
                onReceipt();
            }
        }
    }
}

int Network::error() const
{
    if (socket())
        return socket()->error();
    else
        return myerror;
}

QString Network::errorString() const
{
    if (socket())
        return socket()->errorString();
    else
        return myerrorString;
}

void Network::send(const QByteArray &message)
{
    if (!isConnected())
        return;
    socket()->putChar(message.length()/256);
    socket()->putChar(message.length()%256);
    socket()->write(message);
}

QTcpSocket * Network::socket()
{
    return mysocket;
}

const QTcpSocket * Network::socket() const
{
    return mysocket;
}
