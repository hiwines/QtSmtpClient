#ifndef REXPATTERNS_H
#define REXPATTERNS_H

/// Namespace For Available Patterns
namespace RexPatterns {


/// Pattern to match a valid email
static constexpr const char *Email = "^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]+$";
/// Pattern to match a valid hex-color code
static constexpr const char *HexColor = "^#[0-9ABCDEF]{6}$";

/// Pattern to match a valid Avs-Number
static constexpr const char *AvsNumber = "^[0-9]{3}\\.[0-9]{4}\\.[0-9]{4}\\.[0-9]{2}$";
/// Pattern to match a valid Postal-Account
static constexpr const char *PostalAccount = "^[0-9]{2}-[1-9][0-9]{0,5}-[0-9]$";
/// Pattern to match a valid PVR-Account for Orange Payment Slips
static constexpr const char *PvrAccount = "^(?:01|03)-[1-9][0-9]{0,5}-[0-9]$";
/// Pattern to match a valid Iban
static constexpr const char *Iban = "^[0-9A-Z]{15,34}$";
/// Pattern to match a valid Qr-Iban
static constexpr const char *QrIban = "^(?:CH|LI)\\d{2}(?:30|31)[0-9A-Z]{15}$";
/// Pattern to match a BIC/SWIFT Code as expected from the ISO-20022 standard
static constexpr const char *BicSwiftCode = "^[A-Z]{6,6}[A-Z2-9][A-NP-Z0-9]([A-Z0-9]{3,3}){0,1}$";

/// Pattern to match a valid UID-UFRC Code
static constexpr const char *UidUfrcCode = "^CH-[0-9]{3}\\.[0-9]{1}\\.[0-9]{3}\\.[0-9]{3}\\-[0-9]{1}$";
/// Pattern to match a valid IDI-UST Code
static constexpr const char *IdiUstCode = "^CHE-[0-9]{3}\\.[0-9]{3}\\.[0-9]{3}$";
/// Pattern to match a valid RCC/ZSR Code
static constexpr const char *RccCode = "^[A-Z](?:\\.?[0-9]){6}$";

/// Pattern to match an EAN for EBill purposes
static constexpr const char *EBillEan = "^[0-9]{13}$";

/// Pattern to sanitize the Iso20022 reference text
static constexpr const char *Iso20022Text = "[^\\s0-9A-Za-z\\[\\].,;:!\"#%&<>=@_$£àáâäçèéêëìíîïñòóôöùúûüýßÀÁÂÄÇÈÉÊËÌÍÎÏÒÓÔÖÙÚÛÜÑ]";


} // < RexPatterns NS

#endif // REXPATTERNS_H
