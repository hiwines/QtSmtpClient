#include "smtp_mime.h"

#include <QIODevice>
#include <QMimeDatabase>
#include <QUuid>
#include <QStringBuilder>
#include <QDateTime>
#include <QRegularExpression>
#include <algorithm>
#include "utils/rexpatterns.h"

// namespace usage
using namespace Smtp;



// PRIVATE UTILITY NAMESPACE
namespace {

// Checks whether the given char needs to be escaped for quoted-printable encoding
// \note In order to mitigate errors we decided to encode all chars outside [A-Za-z0-9] space
bool pn_needEscapeForQuotedPrintableEncoding(qint8 input)
{
    // [0-9]
    if (input >= 48 && input <= 57)
        return false;
    // [A-Z]
    if (input >= 65 && input <= 90)
        return false;
    // [a-z]
    if (input >= 97 && input <= 122)
        return false;
    // fallback
    return true;
}
// Split the given text into chunks of the given size
QStringList pn_splitTextIntoChunks(const QString &text, int maxChunkSize)
{
    // fast return for empty text
    if (text.isEmpty())
        return QStringList();
    // fast return for a chunk-size that include all text or an invalid chunk-size
    if (maxChunkSize <= 0 || maxChunkSize >= text.size())
        return QStringList {text};

    // allocate resulting chunks
    QStringList chunks;
    // iterate over all text
    for (const auto charVal : text) {
        // checks whether a new chunk is required
        if (chunks.isEmpty() || chunks.last().size() >= maxChunkSize)
            chunks.append(QString());
        // adds this char to the last chunk
        chunks.last().append(charVal);
    }
    // return computed chunks
    return chunks;
}

} // PRIVATE UTILITY NAMESPACE



// EmailAddress

bool EmailAddress::isValid() const
{
    // whether empty it is not valid
    if (isEmpty())
        return false;
    // ensure the address is valid
    return QRegularExpression(RexPatterns::Email).match(d_email).hasMatch();
}



// MimeUtils

QByteArray MimeUtils::encodeQuotedPrintable(const QString &text)
{
    // fast return for empty strings
    if (text.isEmpty())
        return QByteArray();

    // encode the given text into utf8
    auto utf8Encoded = text.toUtf8();
    // allocate resulting array
    QByteArray output;
    // iterate over all utf8-encoded bytes
    for (const qint8 inputChar : utf8Encoded) {
        // handles charts that needs to be escaped, encoding them with '=' + HEX value
        if (pn_needEscapeForQuotedPrintableEncoding(inputChar))
            output.append('=' % QByteArray(1, inputChar).toHex().toUpper());
        else // fallback for supported chars
            output.append(inputChar);
    }

    // return it
    return output;
}

QByteArray MimeUtils::formatQuotedPrintableIntoLines(const QByteArray &encoded, int maxLineSize)
{
    // fast return for invalid max-line-size
    if (maxLineSize <= 0)
        return encoded;
    // fast return for empty encoded array
    if (encoded.isEmpty())
        return encoded;

    // computes the max amount of allowed source-chars for each line
    // since we need to add "=CRLF" as line separator which counts as 1
    const auto maxSrcCharsForLine = maxLineSize - 1;
    // allocate resulting array
    QByteArray output;
    output.reserve(encoded.size());
    // allocate counting vars
    int lastLineSize = 0;
    // iterate over all bytes from the source
    for (int ix = 0, max = encoded.size(); ix < max; ) {
        // computes the required space for this byte
        int requiredBytes = 1; // default
        // handles '=' escapes that start encoding sequence of 3 bytes
        if (encoded.at(ix) == '=')
            requiredBytes = 3;
        // checks whether we need to use another line
        if ((lastLineSize + requiredBytes) > maxSrcCharsForLine) {
            // adds the line separator
            output += QByteArrayLiteral("=\r\n");
            // reset the line size
            lastLineSize = 0;
        }
        // adds all bytes
        output += encoded.mid(ix, requiredBytes);
        // update counters and index for the consumed bytes
        lastLineSize += requiredBytes;
        ix += requiredBytes;
    }
    // return formatted data
    return output;
}

QByteArray MimeUtils::formatDataIntoLines(const QByteArray &data, int maxLineSize)
{
    // fast return for invalid max-line-size
    if (maxLineSize <= 0)
        return data;
    // fast return for empty encoded array
    if (data.isEmpty())
        return data;

    // allocate resulting array
    QByteArray output;
    output.reserve(data.size());
    // allocate counting vars
    int lastLineSize = 0;
    // iterate over all bytes from the source
    for (const auto byteVal : data) {
        // checks whether we need to use another line
        if (lastLineSize >= maxLineSize) {
            // adds the line separator
            output += QByteArrayLiteral("\r\n");
            // reset the line size
            lastLineSize = 0;
        }
        // append this byte
        output.append(byteVal);
        // update counters
        lastLineSize += 1;
    }
    // return formatted data
    return output;
}

QByteArray MimeUtils::encodeMimeWordQ(const QString &text, int maxWordSize)
{
    // fast return for empty text
    if (text.isEmpty())
        return QByteArray();

    // we cannot predict the size of the word, so we will try until succeeded
    // or until the size of the encoded-text is as small as 1 char only
    // starting with an encoded text equal to the full given text
    int maxEncodedTextSize = text.size();
    // forever
    while (true) {
        // computes text chunks to encode into separate words
        // so we can fulfill given size constraints
        QStringList chunks = pn_splitTextIntoChunks(text, maxEncodedTextSize);
        // allocate resulting words-list and other stats
        QByteArrayList words; // resulting list of words
        int longestWordSize = 0; // size of the longest word
        // iterate over all chunks to compute encoded words
        for (const auto &textChunk : chunks) {
            // starts encoding the word with the prefix
            QByteArray word = QByteArrayLiteral("=?utf-8?Q?");
            // adds the encoded-text to it
            word += encodeQuotedPrintable(textChunk);
            // adds the suffix
            word += QByteArrayLiteral("?=");
            // append it
            words.append(word);
            // update longest word-size
            longestWordSize = std::max<int>(longestWordSize, word.size());
        }
        // checks whether we reached to goal or we cannot go further
        if (longestWordSize <= maxWordSize || maxEncodedTextSize <= 1)
            return words.join(QByteArrayLiteral("\r\n "));
        // otherwise, tries reducing the max encoded text size
        maxEncodedTextSize = (maxEncodedTextSize / 2) + (maxEncodedTextSize % 2);
    }
}

QByteArray MimeUtils::encodeMimeWordB(const QString &text, int maxWordSize)
{
    // fast return for empty text
    if (text.isEmpty())
        return QByteArray();

    // we cannot predict the size of the word, so we will try until succeeded
    // or until the size of the encoded-text is as small as 1 char only
    // starting with an encoded text equal to the full given text
    int maxEncodedTextSize = text.size();
    // forever
    while (true) {
        // computes text chunks to encode into separate words
        // so we can fulfill given size constraints
        QStringList chunks = pn_splitTextIntoChunks(text, maxEncodedTextSize);
        // allocate resulting words-list and other stats
        QByteArrayList words; // resulting list of words
        int longestWordSize = 0; // size of the longest word
        // iterate over all chunks to compute encoded words
        for (const auto &textChunk : chunks) {
            // starts encoding the word with the prefix
            QByteArray word = QByteArrayLiteral("=?utf-8?B?");
            // adds the encoded-text to it
            word += textChunk.toUtf8().toBase64();
            // adds the suffix
            word += QByteArrayLiteral("?=");
            // append it
            words.append(word);
            // update longest word-size
            longestWordSize = std::max<int>(longestWordSize, word.size());
        }
        // checks whether we reached to goal or we cannot go further
        if (longestWordSize <= maxWordSize || maxEncodedTextSize <= 1)
            return words.join(QByteArrayLiteral("\r\n "));
        // otherwise, tries reducing the max encoded text size
        maxEncodedTextSize = (maxEncodedTextSize / 2) + (maxEncodedTextSize % 2);
    }
}

QByteArray MimeUtils::encodeEmailAddress(const EmailAddress &email, int maxWordSize)
{
    // ensure it is valid
    if (!email.isValid())
        return QByteArray();

    // allocate resulting data
    QByteArray encoded;
    // handles an existing owner name
    if (!email.ownerName().isEmpty()) {
        // encode it first using mime-words
        encoded += encodeMimeWordQ(email.ownerName(), maxWordSize);
        // adds a new line to ensure the email do not exceeds the line size
        encoded += QByteArrayLiteral("\r\n <");
    }
    // encode the email address "as-is"
    encoded += email.email().toLatin1();
    // whether the owner name exists, close the wrapping "<>" bracket
    if (!email.ownerName().isEmpty())
        encoded += QByteArrayLiteral(">");
    // return encoded data
    return encoded;
}

QByteArray MimeUtils::encodeEmailAddresses(const EmailAddresses &emails, int maxWordSize)
{
    // ensure the list is not empty
    if (emails.isEmpty())
        return QByteArray();
    // ensure all emails are valid
    for (const auto &email : emails)
        if (!email.isValid())
            return QByteArray();

    // allocate resulting data
    QByteArray encoded;
    // starts encoding all emails
    for (int ix = 0, max = emails.size(); ix < max; ++ix) {
        // adds a prefix after the first one
        if (ix >= 1)
            encoded += QByteArrayLiteral(",\r\n ");
        // encode the email
        encoded += encodeEmailAddress(emails.at(ix), maxWordSize);
    }
    // return encoded data
    return encoded;
}

bool MimeUtils::writeDataToDev(QIODevice &dev, const QByteArray &data)
{
    return (dev.write(data) == data.size());
}



// MimePart

MimePart::MimePart(const QByteArray &contentType)
    : d_contentType(contentType) {}

MimePart::~MimePart() = default;

void MimePart::setContentName(const QString &name)
{
    // replace white-spaces with underscores
    auto parsed = QString(name).replace(QRegularExpression("\\s+"), "_");
    // then remove all un-allowed characters
    parsed.remove(QRegularExpression("[^A-Za-z0-9\\-_\\.]"));
    // apply it
    d_contentName = parsed.toUtf8();
}

bool MimePart::writeStdHeadersToDev(QIODevice &dev, const QByteArray &boundary) const
{
    // ensure the content-type is valid
    if (contentType().isEmpty())
        return false;

    // computes the full content-type arg
    QByteArray fullContentType = QByteArrayLiteral("Content-Type: ") % contentType();
    // optional content-name
    if (!contentName().isEmpty())
        fullContentType += QByteArrayLiteral(";\r\n  name=\"") % contentName() % QByteArrayLiteral("\"");
    // optional charset
    if (!contentCharset().isEmpty())
        fullContentType += QByteArrayLiteral(";\r\n  charset=") % contentCharset();
    // optional boundary
    if (!boundary.isEmpty())
        fullContentType += QByteArrayLiteral(";\r\n  boundary=") % boundary;
    // new line
    fullContentType += QByteArrayLiteral("\r\n");
    // tries writing the content-type into the device
    if (!MimeUtils::writeDataToDev(dev, fullContentType))
        return false;

    // computes the content-transfer-encoding value
    QByteArray encoding; // default none
    // switch over the current encoding
    if (contentTransferEncoding() == ContentBase64Encoded)
        encoding = QByteArrayLiteral("Content-Transfer-Encoding: base64\r\n");
    else if (contentTransferEncoding() == ContentQuotedPrintableEncoded)
        encoding = QByteArrayLiteral("Content-Transfer-Encoding: quoted-printable\r\n");
    // whether relevant, tries writing it
    if (!encoding.isEmpty() && !MimeUtils::writeDataToDev(dev, encoding))
        return false;

    // success
    return true;
}



// MimeText

MimeText::MimeText(const QString &text)
    : MimePart("text/plain"), d_text(text)
{
    // apply the used charset
    setContentCharset("UTF-8");
    // sets transfer encoding
    setContentTransferEncoding(ContentQuotedPrintableEncoded);
}

bool MimeText::writeToDev(QIODevice &dev) const
{
    // ensure the text is valid
    if (text().isEmpty())
        return false;

    // writes standard headers
    if (!writeStdHeadersToDev(dev))
        return false;

    // header-to-content separator
    if (!MimeUtils::writeDataToDev(dev, QByteArrayLiteral("\r\n")))
        return false;
    // writes file content
    auto encodedText = MimeUtils::encodeQuotedPrintable(text());
    if (!MimeUtils::writeDataToDev(dev, MimeUtils::formatQuotedPrintableIntoLines(encodedText)))
        return false;
    // ending new line
    if (!MimeUtils::writeDataToDev(dev, QByteArrayLiteral("\r\n")))
        return false;
    // success
    return true;
}



// MimeHtml

MimeHtml::MimeHtml(const QString &html)
    : MimePart("text/html"), d_html(html)
{
    // apply the used charset
    setContentCharset("UTF-8");
    // sets transfer encoding
    setContentTransferEncoding(ContentQuotedPrintableEncoded);
}

bool MimeHtml::writeToDev(QIODevice &dev) const
{
    // ensure the html is valid
    if (html().isEmpty())
        return false;

    // writes standard headers
    if (!writeStdHeadersToDev(dev))
        return false;

    // header-to-content separator
    if (!MimeUtils::writeDataToDev(dev, QByteArrayLiteral("\r\n")))
        return false;
    // writes file content
    auto encodedText = MimeUtils::encodeQuotedPrintable(html());
    if (!MimeUtils::writeDataToDev(dev, MimeUtils::formatQuotedPrintableIntoLines(encodedText)))
        return false;
    // ending new line
    if (!MimeUtils::writeDataToDev(dev, QByteArrayLiteral("\r\n")))
        return false;
    // success
    return true;
}



// MimeFile

MimeFile::MimeFile(const QByteArray &fileContent, const QString &fileName)
    : MimePart("application/octet-stream"), d_fileContent(fileContent)
{
    // sets the file name
    setContentName(fileName);
    // sets transfer encoding
    setContentTransferEncoding(ContentBase64Encoded);

    // tries applying a more appropriate content-type based on the file name
    auto typesFromFileName = QMimeDatabase().mimeTypesForFileName(contentName());
    if (typesFromFileName.size() == 1)
        setContentType(typesFromFileName.first().name().toLatin1());
}

bool MimeFile::writeToDev(QIODevice &dev) const
{
    // ensure the file has a content and a name
    if (fileName().isEmpty() || fileContent().isEmpty())
        return false;

    // writes standard headers
    if (!writeStdHeadersToDev(dev))
        return false;
    // computes the disposition to encode
    QByteArray dispositionVal;
    // handles known dispositions
    if (disposition() == DisposeAsAttachment)
        dispositionVal = QByteArrayLiteral("Content-Disposition: attachment;\r\n  "
            "filename=\"") % fileName() % QByteArrayLiteral("\"\r\n");
    else if (disposition() == DisposeInline)
        dispositionVal = QByteArrayLiteral("Content-Disposition: inline\r\n");
    // whether relevant tries writing it
    if (!dispositionVal.isEmpty() && !MimeUtils::writeDataToDev(dev, dispositionVal))
        return false;

    // header-to-content separator
    if (!MimeUtils::writeDataToDev(dev, QByteArrayLiteral("\r\n")))
        return false;
    // writes file content
    if (!MimeUtils::writeDataToDev(dev, MimeUtils::formatDataIntoLines(fileContent().toBase64())))
        return false;
    // ending new line
    if (!MimeUtils::writeDataToDev(dev, QByteArrayLiteral("\r\n")))
        return false;
    // success
    return true;
}



// MimeInlineFile

MimeInlineFile::MimeInlineFile(const QByteArray &fileContent, const QString &fileName)
    : MimeFile(fileContent, fileName)
{
    // apply inline disposition
    setDisposition(DisposeInline);
}



// MimeAttachmentFile

MimeAttachmentFile::MimeAttachmentFile(const QByteArray &fileContent, const QString &fileName)
    : MimeFile(fileContent, fileName)
{
    // apply attachment disposition
    setDisposition(DisposeAsAttachment);
}



// MimeMultiPartMixed

MimeMultiPartMixed::MimeMultiPartMixed()
    : MimePart("multipart/mixed") {}

bool MimeMultiPartMixed::writeToDev(QIODevice &dev) const
{
    // ensure the multipart is not empty
    if (isEmpty())
        return false;

    // whether a single part exists, just encode it
    if (d_parts.size() == 1)
        return d_parts.first()->writeToDev(dev);

    // create a unique boundary
    QByteArray boundaryStr = QUuid::createUuid().toRfc4122().toHex();
    QByteArray boundaryPrefix = QByteArrayLiteral("--") % boundaryStr % QByteArrayLiteral("\r\n");
    QByteArray boundarySuffix = QByteArrayLiteral("--") % boundaryStr % QByteArrayLiteral("--\r\n");
    // tries encoding standard headers
    if (!writeStdHeadersToDev(dev, boundaryStr))
        return false;

    // header-to-content separator
    if (!MimeUtils::writeDataToDev(dev, QByteArrayLiteral("\r\n")))
        return false;
    // iterate over all mime parts, to write them
    for (const auto *part : d_parts) {
        // prefix boundary
        if (!MimeUtils::writeDataToDev(dev, boundaryPrefix))
            return false;
        // writes the part
        if (!part->writeToDev(dev))
            return false;
    }
    // boundary suffix
    if (!MimeUtils::writeDataToDev(dev, boundarySuffix))
        return false;
    // success
    return true;
}



// MimeMessage

void MimeMessage::setMessageBodyText(const QString &text)
{
    // whether a body already exists
    if (d_messageBody) {
        // drop it first
        d_multiPart.parts().dropFirst();
        d_messageBody = nullptr;
    }
    // whether a valid text is given
    if (!text.isEmpty()) {
        // create a new body part as first one into the multi-part
        d_messageBody = new MimeText(text);
        d_multiPart.parts().prepend(d_messageBody);
    }
}

QString MimeMessage::messageBodyText() const
{
    // tries upcasting the message body to a MimeText
    if (auto *part = dynamic_cast<MimeText*>(d_messageBody))
        return part->text();
    // fallback
    return QString();
}

void MimeMessage::setMessageBodyHtml(const QString &html)
{
    // whether a body already exists
    if (d_messageBody) {
        // drop it first
        d_multiPart.parts().dropFirst();
        d_messageBody = nullptr;
    }
    // whether a valid html is given
    if (!html.isEmpty()) {
        // create a new body part as first one into the multi-part
        d_messageBody = new MimeHtml(html);
        d_multiPart.parts().prepend(d_messageBody);
    }
}

QString MimeMessage::messageBodyHtml() const
{
    // tries upcasting the message body to a MimeHtml
    if (auto *part = dynamic_cast<MimeHtml*>(d_messageBody))
        return part->html();
    // fallback
    return QString();
}

bool MimeMessage::isValid() const
{
    // ensure there is a valid sender
    if (!d_senderAddress.isValid())
        return false;
    // whether filled, ensure the "reply-to" is valid
    if (!d_replyToAddress.isEmpty() && !d_replyToAddress.isValid())
        return false;
    // ensure there is at least one "To" recipient
    if (d_toAddresses.isEmpty())
        return false;
    // validate "To" recipient
    for (const auto &to : d_toAddresses)
        if (!to.isValid())
            return false;
    // validate "Cc" recipient
    for (const auto &cc : d_ccAddresses)
        if (!cc.isValid())
            return false;
    // ensure there is a valid subject
    if (d_messageSubject.isEmpty())
        return false;
    // ensure there is a valid body
    if (messageBodyText().isEmpty() && messageBodyHtml().isEmpty())
        return false;
    // valid
    return true;
}

bool MimeMessage::writeToDev(QIODevice &dev) const
{
    // ensure this message is valid
    if (!isValid())
        return false;

    // computes the MIME message header
    QByteArray msgHeader = QByteArrayLiteral("MIME-Version: 1.0\r\n");
    // date of this message
    msgHeader += QByteArrayLiteral("Date: ")
        % QDateTime::currentDateTime().toString(Qt::RFC2822Date).toLatin1()
        % QByteArrayLiteral("\r\n");
    // whether valid, encode the "From"
    if (!d_senderAddress.isEmpty())
        msgHeader += QByteArrayLiteral("From: ")
            % MimeUtils::encodeEmailAddress(d_senderAddress) % QByteArrayLiteral("\r\n");
    // whether valid, encode the "Reply-To"
    if (!d_replyToAddress.isEmpty())
        msgHeader += QByteArrayLiteral("Reply-To: ")
            % MimeUtils::encodeEmailAddress(d_replyToAddress) % QByteArrayLiteral("\r\n");
    // whether valid, encode the "To"
    if (!d_toAddresses.isEmpty())
        msgHeader += QByteArrayLiteral("To: ")
            % MimeUtils::encodeEmailAddresses(d_toAddresses) % QByteArrayLiteral("\r\n");
    // whether valid, encode the "Cc"
    if (!d_ccAddresses.isEmpty())
        msgHeader += QByteArrayLiteral("Cc: ")
            % MimeUtils::encodeEmailAddresses(d_ccAddresses) % QByteArrayLiteral("\r\n");
    // whether not empty, encode the subject
    if (!d_messageSubject.isEmpty())
        msgHeader += QByteArrayLiteral("Subject: ")
            % MimeUtils::encodeMimeWordQ(d_messageSubject) % QByteArrayLiteral("\r\n");

    // tries writing the header to dev
    if (!MimeUtils::writeDataToDev(dev, msgHeader))
        return false;
    // tries writing the multi-part content
    if (!d_multiPart.writeToDev(dev))
        return false;
    // END of message
    if (!MimeUtils::writeDataToDev(dev, QByteArrayLiteral("\r\n.\r\n")))
        return false;

    // success
    return true;
}
