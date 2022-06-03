#ifndef SMTP_MIME_H
#define SMTP_MIME_H

#include <QByteArray>
#include <QString>
#include "utils/pointers/scopedptrlist.h"

// fwd declarations
class QIODevice;

// > Smtp NS
namespace Smtp {


/// Named Email Container
class EmailAddress
{
// construction
public:
    /// Builds an Empty and Invalid Email
    EmailAddress() = default;
    /// Builds an Email address with no Name
    EmailAddress(const QString &email)
        : d_email(email) {}
    /// Builds an Email with an optional Name
    EmailAddress(const QString &email, const QString &ownerName)
        : d_email(email), d_name(ownerName) {}
    /// Allow Copy and Assignment
    EmailAddress(const EmailAddress &other) = default;
    EmailAddress& operator=(const EmailAddress &other) = default;

// public interface
public:
    /// Checks whether the address is valid
    bool isValid() const;
    /// Checks whether the address is empty
    inline bool isEmpty() const { return (d_email.isEmpty() && d_name.isEmpty()); }
    /// Gets the email address
    inline const QString& email() const { return d_email; }
    /// Gets the owner name
    inline const QString& ownerName() const { return d_name; }

// private members
private:
    QString d_email;
    QString d_name;
};
/// A list of Email Addresses
using EmailAddresses = QList<EmailAddress>;




// Wrapper for Encoding or General Purpose Utils
// > MimeUtils NS
namespace MimeUtils {


/// Max allowed line size as a general rule
static constexpr const int MaxLineSize = 76;
/// Max mime-word size, choosed to account extra header data
static constexpr const int MaxMimeWordSize = 60;

/// Encode Quoted-Printable Text
QByteArray encodeQuotedPrintable(const QString &text);
/// Format the given Quoted-Printable Text into Lines
QByteArray formatQuotedPrintableIntoLines(const QByteArray &encoded, int maxLineSize = MaxLineSize);
/// Format the given raw-data into Lines
QByteArray formatDataIntoLines(const QByteArray &data, int maxLineSize = MaxLineSize);

/// Encode the given text into a Mime-Word-Q, splitting it over multiple lines if needed
QByteArray encodeMimeWordQ(const QString &text, int maxWordSize = MaxMimeWordSize);
/// Encode the given text into a Mime-Word-B, splitting it over multiple lines if needed
QByteArray encodeMimeWordB(const QString &text, int maxWordSize = MaxMimeWordSize);

/// Encode the given Email-Address splitting it over multiple lines if needed
QByteArray encodeEmailAddress(const EmailAddress &email, int maxWordSize = MaxMimeWordSize);
/// Encode the given list of Email-Addresses splitting them over multiple lines if needed
QByteArray encodeEmailAddresses(const EmailAddresses &emails, int maxWordSize = MaxMimeWordSize);

/// Writes given data into the given device
/// \return True on success, False otherwise
bool writeDataToDev(QIODevice &dev, const QByteArray &data);


} // < MimeUtils NS



/// Generic Mime-Part
class MimePart
{
// public definitions
public:
    /// Content-Transfer-Encoding
    enum ContentTransferEncoding
    {
        ContentNotEncoded,
        ContentBase64Encoded,
        ContentQuotedPrintableEncoded
    };

// construction
public:
    /// Builds a new Generic Mime-Part with the given Content-Type
    explicit MimePart(const QByteArray &contentType);
    /// Builds a new Generic Mime-Part with no Content-Type
    MimePart() = default;
    /// Virtual Destruction
    virtual ~MimePart();

// public interface
public:
    /// Gets the content-type
    inline const QByteArray& contentType() const { return d_contentType; }
    /// Sets the content-type
    inline void setContentType(const QByteArray &type) { d_contentType = type; }

    /// Gets the content-name
    inline const QByteArray& contentName() const { return d_contentName; }
    /// Sets the content-name
    void setContentName(const QString &name);

    /// Gets the content-charset
    inline const QByteArray& contentCharset() const { return d_contentCharset; }
    /// Sets the content-charset
    inline void setContentCharset(const QByteArray &charset) { d_contentCharset = charset; }

    /// Gets the content-transfer-encoding
    inline ContentTransferEncoding contentTransferEncoding() const { return d_contentEncoding; }
    /// Sets the content-transfer-encoding
    inline void setContentTransferEncoding(ContentTransferEncoding enc) { d_contentEncoding = enc; }

    /// Writes mime data into the given device
    /// \return True on success, False otherwise
    virtual bool writeToDev(QIODevice &dev) const = 0;

// protected interface
protected:
    /// Writes standard headers into the given device
    /// \return True on success, False otherwise
    bool writeStdHeadersToDev(QIODevice &dev, const QByteArray &boundary = QByteArray()) const;

// private members
private:
    QByteArray d_contentType;
    QByteArray d_contentName;
    QByteArray d_contentCharset;
    ContentTransferEncoding d_contentEncoding = ContentNotEncoded;
};
/// A List of Mime-Parts
using MimeParts = ScopedPtrList<MimePart>;




/// Text
class MimeText : public MimePart
{
// construction
public:
    /// Builds a Text part with the given Text-Content
    explicit MimeText(const QString &text);

// public interface
public:
    /// Gets the text
    inline const QString& text() const { return d_text; }

    /// Writes the mime data to the device
    bool writeToDev(QIODevice &dev) const override;

// private members
private:
    QString d_text;
};




/// HTML
class MimeHtml : public MimePart
{
// construction
public:
    /// Builds a HTML part with the given Html-Content
    explicit MimeHtml(const QString &html);

// public interface
public:
    /// Gets the html
    inline const QString& html() const { return d_html; }

    /// Writes the mime data to the device
    bool writeToDev(QIODevice &dev) const override;

// private members
private:
    QString d_html;
};




/// Mime Binary File
class MimeFile : public MimePart
{
// public definitions
public:
    /// Available Disposition
    enum Disposition
    {
        NoDisposition,
        DisposeInline,
        DisposeAsAttachment
    };

// construction
public:
    /// Standard Construction giving File-Content and Name
    MimeFile(const QByteArray &fileContent, const QString &fileName);

// public interface
public:
    /// Gets the file content
    inline const QByteArray& fileContent() const { return d_fileContent; }
    /// Gets the file name
    inline const QByteArray& fileName() const { return contentName(); }

    /// Gets the current disposition
    inline Disposition disposition() const { return d_disposition; }
    /// Sets the current disposition
    inline void setDisposition(Disposition d) { d_disposition = d; }

    /// Writes the mime data to the device
    bool writeToDev(QIODevice &dev) const override;

// private members
private:
    QByteArray d_fileContent;
    Disposition d_disposition = NoDisposition;
};




/// Inline Binary File
class MimeInlineFile : public MimeFile
{
// construction
public:
    /// Build an Inline-Disposition File
    MimeInlineFile(const QByteArray &fileContent, const QString &fileName);
};




/// Attachment Binary File
class MimeAttachmentFile : public MimeFile
{
// construction
public:
    /// Build an Inline-Disposition File
    MimeAttachmentFile(const QByteArray &fileContent, const QString &fileName);
};




/// MultiPart/Mixed
class MimeMultiPartMixed : public MimePart
{
// construction
public:
    /// Empty Multi-Part Construction
    MimeMultiPartMixed();
    /// Disable Copy and Assignment
    Q_DISABLE_COPY(MimeMultiPartMixed)

// public interface
public:
    /// Adds a mime-part to this multi-part
    /// \warning The multi-part takes ownership of the given part
    inline void appendPart(MimePart *part) { d_parts.append(part); }
    /// Checks whether there are no parts into this multi-part
    inline bool isEmpty() const { return d_parts.isEmpty(); }
    /// Access all parts
    inline MimeParts& parts() { return d_parts; }
    inline const MimeParts& parts() const { return d_parts; }

    /// Writes the mime data to the device
    /// \warning An empty multi-part will return false
    /// \note A multi-part with a single part, equals to having such part only
    bool writeToDev(QIODevice &dev) const override;

// private members
private:
    MimeParts d_parts;
};




/// Composed Mime-Message
class MimeMessage
{
// construction
public:
    /// Empty Message Construction
    MimeMessage() = default;
    /// Disable Copy and Assignment
    Q_DISABLE_COPY(MimeMessage)

// public interface
public:
    /// Sets the sender address
    inline void setSenderAddress(const EmailAddress &sender) { d_senderAddress = sender; }
    /// Gets the sender address
    inline const EmailAddress& senderAddress() const { return d_senderAddress; }

    /// Sets the reply-to address
    inline void setReplyToAddress(const EmailAddress &replyTo) { d_replyToAddress = replyTo; }
    /// Gets the reply-to address
    inline const EmailAddress& replyToAddress() const { return d_replyToAddress; }

    /// Sets the list of "To" recipients
    inline void setToRecipients(const EmailAddresses &to) { d_toAddresses = to; }
    /// Adds an address to the list of "To" recipients
    inline void addToRecipient(const EmailAddress &to) { d_toAddresses.append(to); }
    /// Gets the list of "To" recipients
    inline const EmailAddresses& toRecipients() const { return d_toAddresses; }

    /// Sets the list of "Cc" recipients
    inline void setCcRecipients(const EmailAddresses &cc) { d_ccAddresses = cc; }
    /// Adds an address to the list of "Cc" recipients
    inline void addCcRecipient(const EmailAddress &cc) { d_ccAddresses.append(cc); }
    /// Gets the list of "Cc" recipients
    inline const EmailAddresses& ccRecipients() const { return d_ccAddresses; }

    /// Sets the message subject
    inline void setMessageSubject(const QString &text) { d_messageSubject = text; }
    /// Gets the message subject
    inline const QString& messageSubject() const { return d_messageSubject; }

    /// Sets the message body as a plan text
    void setMessageBodyText(const QString &text);
    /// Gets the plain-text message body
    QString messageBodyText() const;
    /// Sets the message body as html text
    void setMessageBodyHtml(const QString &html);
    /// Gets the html message body
    QString messageBodyHtml() const;

    /// Adds a new mime part to the message
    /// \note The message takes ownership of the given part
    inline void addMimePart(MimePart *part) { d_multiPart.appendPart(part); }

    /// Checks whether the message is valid
    /// \note To be valid a message must have valid addresses with at least one sender
    ///     and a "To" recipient, a valid subject, and a valid message body
    bool isValid() const;

    /// Writes the mime message data to the device
    /// \warning An invalid message will return false
    bool writeToDev(QIODevice &dev) const;

// private members
private:
    EmailAddress d_senderAddress;
    EmailAddress d_replyToAddress;
    EmailAddresses d_toAddresses;
    EmailAddresses d_ccAddresses;
    QString d_messageSubject;
    MimePart *d_messageBody = nullptr;
    MimeMultiPartMixed d_multiPart;
};


} // < Smtp NS

#endif // MIMEPART_H
