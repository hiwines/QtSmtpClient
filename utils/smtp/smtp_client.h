#ifndef SMTP_CLIENT_H
#define SMTP_CLIENT_H

#include <QObject>
#include <QSslSocket>
#include "utils/smtp/smtp_mime.h"
#include "utils/macros.h"

// > Smpt NS
namespace Smtp {


/// Basic Client
class Client : public QObject
{
    Q_OBJECT

// public definitions
public:
    /// Authentication Methods
    enum AuthMethod
    {
        AuthNone,
        AuthPlain,
        AuthLogin,
        AuthCramMd5
    };
    /// Supported Connection Types
    enum ConnectionType
    {
        UnknownConnection,
        TcpConnection,
        SslConnection,
        TlsConnection
    };

// construction
public:
    // Ctor
    explicit Client(QObject *parent = nullptr);
    // Dtor
    ~Client();

// public interface
public:
    /// Sets the server hostname
    void setServerHost(const QString &host);
    /// Gets the server hostname
    const QString& serverHost() const;

    /// Sets the server port
    void setServerPort(quint16 port);
    /// Gets the server port
    quint16 serverPort() const;

    /// Sets the client hostname
    /// \default As default the local hostname is used
    void setClientHost(const QString &host);
    /// Gets the client hostname
    const QString& clientHost() const;

    /// Sets the connection-type to use
    /// \note For SSL connections this will use the default peer-verification mode
    void setConnectionType(ConnectionType type);
    /// Sets the connection-type to use and the peer-verification mode
    /// \param vmode Peer-Verification-Mode for SSL connection only
    /// \param ignoreSslErrors True to setup the SSL socket so it will ignore ssl-related errors
    void setConnectionType(ConnectionType type, QSslSocket::PeerVerifyMode vmode, bool ignoreSslErrors);
    /// Gets the used connection-type
    ConnectionType connectionType() const;

    /// Sets the username of the mail account
    /// \note Whether "AuthNone" this will set the AuthMode to "AuthPlain"
    void setAccountUser(const QString &user);
    /// Gets the account username
    const QString& accountUsername() const;
    /// Sets the password of the mail account
    /// \note Whether "AuthNone" this will set the AuthMode to "AuthPlain"
    void setAccountPassword(const QString &password);
    /// Gets the account password
    const QString& accountPassword() const;

    /// Sets the authentication method to use
    void setAuthMethod(AuthMethod method);
    /// Gets the used authentication method
    AuthMethod authMethod() const;

    /// Sets the connection timeout interval
    /// \default As default 15 seconds
    void setConnectionTimeout(int msec);
    /// Gets the connection timout interval
    int connectionTimeout() const;

    /// Sets the timout interval to use while waiting for the response
    /// \default As default 15 seconds
    void setResponseTimeout(int msec);
    /// Gets the response timout interval
    int responseTimeout() const;

    /// Sets the timout interval to use while sending a message
    /// \default As default 60 seconds
    void setSendTimeout(int msec);
    /// Gets the send timout interval
    int sendTimeout() const;

    /// Enable/Disable Socket Traffic Log
    void setSocketTrafficLogEnabled(bool on);

    /// Tries to connect and authenticate to the server
    /// \return True on success, False otherwise
    /// \note This is required before you are allowed to send messages
    bool connectToServer();
    /// Tries sending a mime-message
    /// \warning Whether an smtp protocol failure occur, the client will be disconnected
    ///     in order to avoid errors due to following misbehaving interactions
    /// \return True on success, False otherwise
    bool sendMessage(const MimeMessage &msg) const;
    /// Closes the open connection (if any)
    /// \note Whether connected, the client will disconnect itself on destruction
    void closeConnection();

// private members
private:
    PRIVATE_DATA_PTR(d)
};


} // < Smpt NS

#endif // SMTP_CLIENT_H
