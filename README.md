# QtSmtpClient

Basic SMTP Client Powered By Qt and C++

Usage Example

```cpp
// client allocation
Smtp::Client smtpClient;
// client setup
smtpClient.setServerHost("Host");
smtpClient.setServerPort(port);
smtpClient.setConnectionType(Smtp::Client::TlsConnection);
smtpClient.setAccountUser("Username");
smtpClient.setAccountPassword("Password");
smtpClient.setAuthMethod(Smtp::Client::AuthLogin);
// tries connecting to the server
if (!smtpClient.connectToServer())
    return; // error

// create the email object
Smtp::MimeMessage mail;
// sender and recipient
mail.setSenderAddress(smtpClient.accountUsername());
mail.addToRecipient("Recipient Email");
// subject and body
mail.setMessageSubject("This is the EMail Subject");
mail.setMessageBodyText("This is the EMail Body");
// attach a pdf file
mail.addMimePart(new Smtp::MimeAttachmentFile(pdfFile.readAll(), "FileName.pdf"));
// tries sending the mail
if (!smtpClient.sendMessage(mail))
    return; // error
```
