#include "smtp_client.h"

#include <QMessageAuthenticationCode>
#include <QHostInfo>
#include <QStringBuilder>
#include <QBuffer>
#include "utils/rtloghandler.h"

// namespace usage
using namespace Smtp;



// Client

struct Smtp::Client::PrivateData
{
    // Client Status
    enum Status
    {
        ST_Disconnected,
        ST_Connected
    };

    // Members
    Status status = ST_Disconnected;
    QString serverHost;
    quint16 serverPort = 0;
    QString clientHost = QHostInfo::localHostName();
    ConnectionType connectionType = UnknownConnection;
    QString accountUsername, accountPassword;
    AuthMethod authMethod = AuthNone;

    int connectionTimout = 15000;
    int responseTimeout = 15000;
    int sendTimeout = 60000;
    QTcpSocket *socket = nullptr;
    bool logSocketTraffic = false;
};

// PRIVATE UTILITY NAMESPACE
namespace {

// Log the error and fail (return false)
bool pn_fail(const QString &err, const CallContext &ctx)
{
    // log the error as a warning
    RTLogHandler(ctx, RTLogHandler::Warning, err);
    // fail
    return false;
}
// Tries closing the socket and fail
bool pn_closeAndFail(Smtp::Client::PrivateData *d)
{
    // update the status as disconnected
    d->status = Smtp::Client::PrivateData::ST_Disconnected;
    // close the socket
    d->socket->close();
    // fail
    return false;
}

// Log socket traffic
void pn_logTraffic(Smtp::Client::PrivateData *d, const QByteArray &who, const QByteArray &msg)
{
    // fast return whether log is disabled
    if (!d->logSocketTraffic)
        return;
    // log traffic
    RT_DEBUG("log-traffic > %1: %2") % who % msg;
}

// Verify that is it allowed to send a message over the socket
bool pn_isAllowedToSend(Smtp::Client::PrivateData *d)
{
    // before we write data, ensure there is nothing available to read
    // cause otherwise it will be read as response to this new message
    if (d->socket->canReadLine()) {
        // report such error
        pn_fail("send fail, found unexpected data available to be read, was:", CALL_CONTEXT);
        // extract all data from it
        while (d->socket->canReadLine()) {
            // extract the line and log it
            pn_fail(QStringLiteral(" > msg-line: %1")
                .arg(QString::fromUtf8(d->socket->readLine())), CALL_CONTEXT);
        }
        // fail
        return false;
    }
    // allowed
    return true;
}
// Send a message using the socket, ensuring there is no pending data to read
bool pn_sendMessage(Smtp::Client::PrivateData *d, const QByteArray &data)
{
    // validate send
    if (!pn_isAllowedToSend(d))
        return false;
    // optional log for traffic
    pn_logTraffic(d, "C", data);
    // writes given data to the socket
    d->socket->write(data % QByteArrayLiteral("\r\n"));
    // success
    return true;
}
// Send a message using the socket, ensuring there is no pending data to read
bool pn_sendMessage(Smtp::Client::PrivateData *d, const Smtp::MimeMessage &msg)
{
    // validate send
    if (!pn_isAllowedToSend(d))
        return false;

    // allocate a buffer to collect data to send
    QByteArray dataToSend;
    QBuffer writer (&dataToSend);
    // open it to write over it
    writer.open(QBuffer::WriteOnly);
    // write message to the buffer
    if (!msg.writeToDev(writer))
        return false;
    // close the writer
    writer.close();

    // optional log for traffic
    pn_logTraffic(d, "C", dataToSend.toBase64());
    // writes given data to the socket
    d->socket->write(dataToSend);
    // success
    return true;
}

// Wait for a response with the given Code over the socket, returning the message body
bool pn_waitForResponse(
    Smtp::Client::PrivateData *d, const QByteArray &expectedCode, QByteArray &receivedMessageBody)
{
    // ensure the message body is empty
    receivedMessageBody = QByteArray();

    // forever
    while (true) {
        // wait for something to read
        if (!d->socket->waitForReadyRead(d->responseTimeout))
            return pn_fail("unable to wait for server response, connection timout", CALL_CONTEXT);
        // while there are lines to read
        while (d->socket->canReadLine()) {
            // read a full line
            auto line = d->socket->readLine().trimmed();
            // optional log for traffic
            pn_logTraffic(d, "S", line);
            // extract the code and the code-to-message separator
            auto code = line.left(3); // first 3 bytes
            auto codeToMsgSep = line.mid(3, 1); // 4th byte
            // account the message with an empty separator only
            if (codeToMsgSep == QByteArrayLiteral(" ")) {
                // ensure the code is the expected one
                if (code != expectedCode)
                    return pn_fail(QStringLiteral("invalid response, expected %1, received: %2")
                        .arg(QString::fromUtf8(expectedCode), QString::fromUtf8(line)), CALL_CONTEXT);
                // valid response, fill the message body
                receivedMessageBody = line.mid(4);
                // success
                return true;
            }
        }
    }
}
// Overloaded version to ignore the message body
bool pn_waitForResponse(Smtp::Client::PrivateData *d, const QByteArray &expectedCode)
{
    // allocate the message body
    QByteArray ignoredMsgBody;
    // wait for the response
    return pn_waitForResponse(d, expectedCode, ignoredMsgBody);
}

// Send a message and wait for the given response code
inline bool pn_sendAndWaitFor(
    Smtp::Client::PrivateData *d, const QByteArray &dataToSend, const QByteArray &expectedResCode)
{
    // tries to send the given message and wait for the given response code
    return (pn_sendMessage(d, dataToSend) && pn_waitForResponse(d, expectedResCode));
}

} // PRIVATE UTILITY NAMESPACE

Client::Client(QObject *parent)
    : QObject(parent), d(new PrivateData()) {}

Client::~Client()
{
    // ensure the connection is closed
    closeConnection();
    // free resources
    delete d;
}

void Client::setServerHost(const QString &host)
{
    // ensure the client is disconnected
    if (d->status != PrivateData::ST_Disconnected)
        return;
    // update setting
    d->serverHost = host;
}

const QString& Client::serverHost() const
{
    return d->serverHost;
}

void Client::setServerPort(quint16 port)
{
    // ensure the client is disconnected
    if (d->status != PrivateData::ST_Disconnected)
        return;
    // update setting
    d->serverPort = port;
}

quint16 Client::serverPort() const
{
    return d->serverPort;
}

void Client::setClientHost(const QString &host)
{
    // ensure the client is disconnected
    if (d->status != PrivateData::ST_Disconnected)
        return;
    // update setting
    d->clientHost = host;
}

const QString& Client::clientHost() const
{
    return d->clientHost;
}

void Client::setConnectionType(ConnectionType connectionType)
{
    // ensure this is called once
    RT_CHECK(d->socket == nullptr);
    // ensure the connection type is valid
    RT_CHECK(connectionType != UnknownConnection);

    // store the connection type
    d->connectionType = connectionType;

    // TCP connection
    if (connectionType == TcpConnection) {
        // builds the socket
        d->socket = new QTcpSocket(this);

    // SSL connection
    } else if (connectionType == SslConnection || connectionType == TlsConnection) {
        // builds the socket
        d->socket = new QSslSocket(this);
    }
}

void Client::setConnectionType(
    ConnectionType connectionType, QSslSocket::PeerVerifyMode vmode, bool ignoreSslErrors)
{
    // ensure this is called once
    RT_CHECK(d->socket == nullptr);
    // ensure the connection type is valid
    RT_CHECK(connectionType != UnknownConnection);

    // store the connection type
    d->connectionType = connectionType;

    // TCP connection
    if (connectionType == TcpConnection) {
        // builds the socket
        d->socket = new QTcpSocket(this);

    // SSL connection
    } else if (connectionType == SslConnection || connectionType == TlsConnection) {
        // builds the socket
        d->socket = new QSslSocket(this);
        // sets the expected peer verification mode
        static_cast<QSslSocket*>(d->socket)->setPeerVerifyMode(vmode);
        // setup the socket to igore errors according to args
        if (ignoreSslErrors)
            static_cast<QSslSocket*>(d->socket)->ignoreSslErrors();
    }
}

Client::ConnectionType Client::connectionType() const
{
    return d->connectionType;
}

void Client::setAccountUser(const QString &user)
{
    // ensure the client is disconnected
    if (d->status != PrivateData::ST_Disconnected)
        return;

    // update credentials
    d->accountUsername = user;

    // update the auth-mode to plain whether none is set
    if (d->authMethod == AuthNone)
        d->authMethod = AuthPlain;
}

const QString& Client::accountUsername() const
{
    return d->accountUsername;
}

void Client::setAccountPassword(const QString &password)
{
    // ensure the client is disconnected
    if (d->status != PrivateData::ST_Disconnected)
        return;

    // update credentials
    d->accountPassword = password;

    // update the auth-mode to plain whether none is set
    if (d->authMethod == AuthNone)
        d->authMethod = AuthPlain;
}

const QString& Client::accountPassword() const
{
    return d->accountPassword;
}

void Client::setAuthMethod(AuthMethod method)
{
    // ensure the client is disconnected
    if (d->status != PrivateData::ST_Disconnected)
        return;
    // update setting
    d->authMethod = method;
}

Client::AuthMethod Client::authMethod() const
{
    return d->authMethod;
}

void Client::setConnectionTimeout(int msec)
{
    // ensure the client is disconnected
    if (d->status != PrivateData::ST_Disconnected)
        return;
    // update setting
    d->connectionTimout = msec;
}

int Client::connectionTimeout() const
{
    return d->connectionTimout;
}

void Client::setResponseTimeout(int msec)
{
    // ensure the client is disconnected
    if (d->status != PrivateData::ST_Disconnected)
        return;
    // update setting
    d->responseTimeout = msec;
}

int Client::responseTimeout() const
{
    return d->responseTimeout;
}

void Client::setSendTimeout(int msec)
{
    // ensure the client is disconnected
    if (d->status != PrivateData::ST_Disconnected)
        return;
    // update setting
    d->sendTimeout = msec;
}

int Client::sendTimeout() const
{
    return d->sendTimeout;
}

void Client::setSocketTrafficLogEnabled(bool on)
{
    d->logSocketTraffic = on;
}

bool Client::connectToServer()
{
    // ensure it is disconnected
    if (d->status != PrivateData::ST_Disconnected)
        return pn_fail("connection not allowed, client is already connected", CALL_CONTEXT);
    // ensure the connection type was set
    if (d->socket == nullptr)
        return pn_fail("unable to connect, call setConnectionType first", CALL_CONTEXT);
    // ensure server host and port were set
    if (serverHost().isEmpty() || serverPort() == 0)
        return pn_fail("unable to connect, missing server host / port", CALL_CONTEXT);
    // ensure client host was set
    if (clientHost().isEmpty())
        return pn_fail("unable to connect, missing client host", CALL_CONTEXT);
    // ensure username and password are set for a valid auth-method
    if (authMethod() != AuthNone && (accountUsername().isEmpty() || accountPassword().isEmpty()))
        return pn_fail("unable to connect, missing account credentials", CALL_CONTEXT);

    // encrypted connection
    if (d->connectionType == SslConnection) {
        // tries connecting to the host
        static_cast<QSslSocket*>(d->socket)->connectToHostEncrypted(serverHost(), serverPort());
    // standard connection
    } else {
        // tries connecting to the host
        d->socket->connectToHost(serverHost(), serverPort());
    }
    // wait for the socket to be connected
    if (!d->socket->waitForConnected(connectionTimeout()))
        return pn_fail(QStringLiteral("unable to connect, connection timeout with error: %1")
            .arg(d->socket->errorString()), CALL_CONTEXT);
    // now wait for the server response
    if (!pn_waitForResponse(d, "220"))
        return pn_closeAndFail(d);

    // send a EHLO/HELO message to the server
    if (!pn_sendAndWaitFor(d, QByteArrayLiteral("EHLO ") % clientHost().toLatin1(), "250"))
        return pn_closeAndFail(d);

    // TLS connections now need to upgrato into encrypted
    if (d->connectionType == TlsConnection) {
        // send a request to start TLS handshake
        if (!pn_sendAndWaitFor(d, QByteArrayLiteral("STARTTLS"), "220"))
            return pn_closeAndFail(d);
        // tries to start encrypted connection
        static_cast<QSslSocket*>(d->socket)->startClientEncryption();
        // wait for encrypted mode
        if (!static_cast<QSslSocket*>(d->socket)->waitForEncrypted(connectionTimeout())) {
            // report the error
            pn_fail(QStringLiteral("unable to upgrade to encrypted mode, error: %1")
                .arg(d->socket->errorString()), CALL_CONTEXT);
            // close and fail
            return pn_closeAndFail(d);
        }

        // another EHLO for the encrypted mode
        if (!pn_sendAndWaitFor(d, QByteArrayLiteral("EHLO ") % clientHost().toLatin1(), "250"))
            return pn_closeAndFail(d);
    }

    // connection to the server succeeded, not try logging in
    // switching over the choosed authentication mode

    // AuthPlain
    if (authMethod() == AuthPlain) {
        // sending command: AUTH PLAIN base64('\0' + username + '\0' + password)
        auto encodedUserAndPswd = QString(QLatin1Char('\0') % accountUsername()
            % QLatin1Char('\0') + accountPassword()).toLatin1().toBase64();
        if (!pn_sendAndWaitFor(d, QByteArrayLiteral("AUTH PLAIN ") % encodedUserAndPswd, "235"))
            return pn_closeAndFail(d);

    // AuthLogin
    } else if (authMethod() == AuthLogin) {
        // sending command: AUTH LOGIN
        if (!pn_sendAndWaitFor(d, QByteArrayLiteral("AUTH LOGIN"), "334"))
            return pn_closeAndFail(d);
        // send the username as base-64 encoded
        if (!pn_sendAndWaitFor(d, accountUsername().toLatin1().toBase64(), "334"))
            return pn_closeAndFail(d);
        // send the password as base-64 encoded
        if (!pn_sendAndWaitFor(d, accountPassword().toUtf8().toBase64(), "235"))
            return pn_closeAndFail(d);

    // AuthCramMd5
    } else if (authMethod() == AuthCramMd5) {
        // sending command: AUTH CRAM-MD5
        if (!pn_sendMessage(d, QByteArrayLiteral("AUTH CRAM-MD5")))
            return pn_closeAndFail(d);
        // wait for server ack, reading the message which will be the challenge value
        QByteArray msgBody;
        if (!pn_waitForResponse(d, "334", msgBody))
            return pn_closeAndFail(d);
        // compute the message autentication code
        QMessageAuthenticationCode authCode (QCryptographicHash::Md5);
        authCode.setKey(accountPassword().toLatin1());
        authCode.addData(QByteArray::fromBase64(msgBody));
        // compute authentication token to send: "<username> <auth-code>"
        QByteArray authToken = accountUsername().toLatin1() % ' ' % authCode.result().toHex();
        // send it as base-64 encoded
        if (!pn_sendAndWaitFor(d, authToken.toBase64(), "235"))
            return pn_closeAndFail(d);
    }

    // client is now connected
    d->status = PrivateData::ST_Connected;
    // success
    return true;
}

void Client::closeConnection()
{
    // ensure the client is connected
    if (d->status != PrivateData::ST_Connected)
        return;
    // just close the connection resetting the status
    pn_closeAndFail(d);
}

bool Client::sendMessage(const MimeMessage &msg) const
{
    // ensure the message is valid
    if (!msg.isValid())
        return pn_fail("unable to send, message is not valid", CALL_CONTEXT);
    // ensure the client is connected
    if (d->status != PrivateData::ST_Connected)
        return pn_fail("unable to send, client is not connected", CALL_CONTEXT);

    // send the sender
    auto senderMsg = QStringLiteral("MAIL FROM:<%1>").arg(msg.senderAddress().email()).toLatin1();
    if (!pn_sendAndWaitFor(d, senderMsg, "250"))
        return pn_closeAndFail(d);
    // iterate over all "To" recipient
    for (const auto &to : msg.toRecipients()) {
        // compute the message
        auto toMsg = QStringLiteral("RCPT TO:<%1>").arg(to.email()).toLatin1();
        if (!pn_sendAndWaitFor(d, toMsg, "250"))
            return pn_closeAndFail(d);
    }
    // iterate over all "Cc" recipient
    for (const auto &cc : msg.ccRecipients()) {
        // compute the message
        auto ccMsg = QStringLiteral("RCPT TO:<%1>").arg(cc.email()).toLatin1();
        if (!pn_sendAndWaitFor(d, ccMsg, "250"))
            return pn_closeAndFail(d);
    }

    // data command to start sending the message
    if (!pn_sendAndWaitFor(d, QByteArrayLiteral("DATA"), "354"))
        return pn_closeAndFail(d);
    // writes the mime-message to the socket
    if (!pn_sendMessage(d, msg)) {
        // report the error
        pn_fail("unexpected error, unable to write msg to socket", CALL_CONTEXT);
        // close and fail
        return pn_closeAndFail(d);
    }
    // wait for the server ack
    if (!pn_waitForResponse(d, "250"))
        return pn_closeAndFail(d);

    // success
    return true;
}
