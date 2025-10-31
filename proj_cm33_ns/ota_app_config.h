/*****************************************************************************
* File Name        : ota_app_config.h
*
* Description      : This file contains the OTA application level configuration macros.
*
* Related Document : See README.md
*
*******************************************************************************
* (c) 2024-2025, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
* 
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

#ifndef OTA_APP_CONFIG_H__
#define OTA_APP_CONFIG_H__ 1

/**********************************************
 * Wi-Fi connection parameters
 *********************************************/

/* Name of the Wi-Fi network */
//#define WIFI_SSID                            ""

/* Password for the Wi-Fi network */
//#define WIFI_PASSWORD                        ""

/* Security type of the Wi-Fi access point. See 'cy_wcm_security_t' structure
 * in "cy_wcm.h" for more details.
 */
//#define WIFI_SECURITY                        (CY_WCM_SECURITY_WPA2_AES_PSK)

/* Maximum Wi-Fi re-connection limit. */
//#define MAX_WIFI_CONN_RETRIES                (3)

/* Wi-Fi re-connection time interval in milliseconds. */
//#define WIFI_CONN_RETRY_INTERVAL_MS          (5000)


/**********************************************
 * HTTP connection parameters
 *********************************************/

#define OTA_CONNECTION_TYPE                  CY_OTA_CONNECTION_HTTPS

#define OTA_UPDATE_FLOW                      CY_OTA_JOB_FLOW

#define OTA_HTTP_SERVER                      ""

#define OTA_HTTP_SERVER_PORT                 (443)

#define ENABLE_TLS                           1

//#define OTA_HTTP_DATA_FILE                   "/pse_ota_update.tar"
#define OTA_HTTP_DATA_FILE                   "/ota_update.tar"

//#define OTA_HTTP_JOB_FILE                    "/pse_ota_update.json"
#define OTA_HTTP_JOB_FILE                    "/ota_update.json"

#define S3_SNI								""

/**********************************************
 * Certificates and Keys - TLS Mode only
 *********************************************/
/* Root CA Certificate -
   Must include the PEM header and footer:

        "-----BEGIN CERTIFICATE-----\n" \
        ".........base64 data.......\n" \
        "-----END CERTIFICATE-------\n"
*/
#define ROOT_CA_CERTIFICATE    ""


/* Client Certificate
   Must include the PEM header and footer:

        "-----BEGIN CERTIFICATE-----\n" \
        ".........base64 data.......\n" \
        "-----END CERTIFICATE-------\n"
*/

/*
#define CLIENT_CERTIFICATE  \
         "-----BEGIN CERTIFICATE-----\n" \
         ".........base64 data.......\n" \
         "-----END CERTIFICATE-----\n";
*/

/* Private Key
   Must include the PEM header and footer:

        "-----BEGIN RSA PRIVATE KEY-----\n" \
        "...........base64 data.........\n" \
        "-----END RSA PRIVATE KEY-------\n"
*/
/*
#define CLIENT_KEY     \
        "-----BEGIN RSA PRIVATE KEY-----\n" \
        "...........base64 data.........\n" \
        "-----END RSA PRIVATE KEY-------\n";
*/
#define CLIENT_CERTIFICATE 	""
#define CLIENT_KEY			""

#endif /* OTA_APP_CONFIG_H__ */


