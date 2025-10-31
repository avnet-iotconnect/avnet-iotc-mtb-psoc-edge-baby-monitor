/*****************************************************************************
* File Name        : ota_test_console.c
*
* Description      : User defines for the OTA library
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

#ifndef CY_OTA_CONFIG_H__
#define CY_OTA_CONFIG_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup group_ota_config
 * \{
 */
/**
 * @brief Initial time for checking for OTA updates.
 *
 * This is used to start the timer for the initial OTA update check after calling cy_ota_agent_start().
 */
#define CY_OTA_INITIAL_CHECK_SECS           (5)            /* 10 seconds */

/**
 * @brief Next time for checking for OTA updates.
 *
 * This is used to restart the timer after an OTA update check in the OTA Agent.
 */
#define CY_OTA_NEXT_CHECK_INTERVAL_SECS     (1 * 60)  /* 1 minute between checks for testing. */

/**
 * @brief Retry time which checking for OTA updates.
 *
 * This is used to restart the timer after failing to contact the server during an OTA update check.
 */
#define CY_OTA_RETRY_INTERVAL_SECS          (5)             /* 5 seconds between retries after an error. */

/**
 * @brief Length of time to check for downloads.
 *
 * The OTA Agent wakes up, connects to server, and waits this much time before disconnecting.
 * This allows the OTA Agent to be inactive for long periods of time, only checking for short periods.
 * Use 0x00 to continue checking once started.
 */
#define CY_OTA_CHECK_TIME_SECS              (30)       /* 30 seconds for testing. */

/**
 * @brief Expected maximum download time between each OTA packet arrival.
 *
 * This is used to verify that the download occurs in a reasonable time frame.
 * Set to 0 to disable this check.
 */
#define CY_OTA_PACKET_INTERVAL_SECS         (0)             /* Default disabled. */

/**
 * @brief Length of time to check for getting Job document.
 *
 * The OTA Agent wakes up, connects to broker/server, and waits this much time before disconnecting.
 * This allows the OTA Agent to be inactive for long periods of time, only checking for short periods.
 * Use 0x00 to continue checking once started.
 */
#define CY_OTA_JOB_CHECK_TIME_SECS           (30)               /* 30 seconds. */

/**
 * @brief Length of time to check for getting the OTA image data.
 *
 * After getting the Job (or during a direct download), this is the amount of time to wait before
 * canceling the download.
 * Use 0x00 to disable.
 */
#define CY_OTA_DATA_CHECK_TIME_SECS          (10 * 60)           /* 5 minutes for testing */

/**
 * @brief Number of retries when attempting an OTA update.
 *
 * This is used to determine # retries when attempting an OTA update.
 */
#define CY_OTA_RETRIES                      (3)             /* Retry entire process 3 times. */

/**
 * @brief Number of retries when attempting to contact the server.
 *
 * This is used to determine # retries when connecting to the server during an OTA update check.
 */
#define CY_OTA_CONNECT_RETRIES              (3)             /* 3 server connect retries  */

/**
 * @brief Number of OTA download retries
 *
 * Retry count for attempts at downloading the OTA Image
 */
#define CY_OTA_MAX_DOWNLOAD_TRIES           (3)             /* 3 download OTA Image retries */

/**
 * @brief HTTP timeout for sending messages
 *
 */
#define CY_OTA_HTTP_TIMEOUT_SEND            (700)         /* 700 miliseconds send timeout. */

/**
 * @brief HTTP timeout for receiving messages
 *
 */
#define CY_OTA_HTTP_TIMEOUT_RECEIVE         (700)         /* 700 miliseconds receive timeout. */


/**********************************************************************
 * Message Defines
 **********************************************************************/

/**
 * @brief Update Successful message.
 *
 * Used with sprintf() to create the RESULT message to the Broker/server.
 */
#define CY_OTA_RESULT_SUCCESS               "Success"

/**
* @brief Update Failure message
*
* Used with sprintf() to create the RESULT message to the Broker/server.
*/
#define CY_OTA_RESULT_FAILURE               "Failure"

/**
 * @brief Default Job document name.
 *
 * Name of the update JSON file for HTTP.
 */
#ifndef CY_OTA_HTTP_JOB_FILE
#define CY_OTA_HTTP_JOB_FILE               "/ota_update.json"
#endif

/**
 * @brief Default OTA image file name.
 *
 * Name of the OTA image for HTTP.
 */
#ifndef CY_OTA_HTTP_DATA_FILE
#define CY_OTA_HTTP_DATA_FILE              "/ota-update.bin"
#endif

/**
 * @brief Device message to the Publisher to ask for a chunk of data.
 * *
 * Used with sprintf() to insert values at runtime.
 *   Current Application Version
 *   UniqueTopicName
 *   FileName
 *   Offset
 *   Size
 * Override if desired by defining in cy_ota_config.h.
 */
#define CY_OTA_DOWNLOAD_CHUNK_REQUEST \
"{\
\"Message\":\"Request Data Chunk\", \
\"Manufacturer\": \"Express Widgits Corporation\", \
\"ManufacturerID\": \"EWCO\", \
\"ProductID\": \"Easy Widgit\", \
\"SerialNumber\": \"ABC213450001\", \
\"BoardName\": \"CY8CPROTO_062_4343W\", \
\"Version\": \"%d.%d.%d\", \
\"UniqueTopicName\": \"%s\", \
\"Filename\": \"%s\", \
\"Offset\": \"%ld\", \
\"Size\": \"%ld\"\
}"

/**
 * @brief Device JSON document to respond to the HTTP server.
 *
 * Used with sprintf() to create the JSON message.
 * Override if desired by defining in cy_ota_config.h.
 */
#define CY_OTA_HTTP_RESULT_JSON \
"{\
\"Message\":\"%s\", \
\"File\":\"%s\" \
}"

/**
 * @brief HTTP GET template.
 *
 * Used with sprintf() to create the GET request for the HTTP server.
 */
#ifndef CY_OTA_HTTP_GET_TEMPLATE
#define CY_OTA_HTTP_GET_TEMPLATE \
    "GET %s HTTP/1.1\r\n" \
    "Host: %s:%d \r\n" \
    "\r\n"
#endif

/**
 * @brief HTTP GET Range template.
 *
 * Used with sprintf() to create the GET request for the HTTP server
 * when requesting a range of data.
 */
#ifndef CY_OTA_HTTP_GET_RANGE_TEMPLATE
#define CY_OTA_HTTP_GET_RANGE_TEMPLATE \
    "GET %s HTTP/1.1\r\n" \
    "Host: %s:%d \r\n" \
    "Range: bytes=%ld-%ld \r\n" \
    "\r\n"
#endif

/**
 * @brief HTTP POST template.
 *
 * Used with sprintf() to create the POST message for the HTTP server.
 */
#ifndef CY_OTA_HTTP_POST_TEMPLATE
#define CY_OTA_HTTP_POST_TEMPLATE \
    "POST %s HTTP/1.1\r\n" \
    "Content-Length:%ld \r\n" \
    "\r\n%s"
#endif


/** \} group_ota_config */

#ifdef __cplusplus
    }
#endif

#endif /* CY_OTA_CONFIG_H__ */

/** \} group_cy_ota */
