/* SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com>, Shu Liu <shu.liu@avnet.com> et al.
 */

#include "cybsp.h"
#include <string.h>

#include "cy_syslib.h" // for Cy_SysLib_GetUniqueId

#include "FreeRTOS.h"

#include "retarget_io_init.h"
#include "ipc_communication.h"

#include "wifi_config.h"
#include "wifi_app.h"

#include "iotconnect.h"
#include "iotc_mtb_time.h"

#include "app_config.h"

/* OTA storage api */
#include "cy_ota_storage_api.h"
//#include "lwip/netif.h"
#include "ota_app_config.h"

/////////////////////////////////////////////////////////////////////////////

#define APP_VERSION		"1.1.1"

static bool is_demo_mode = false;
static int reporting_interval = 2000;

//OTA
static bool ota_in_progress = false;
cy_rslt_t initialize_ota(void);
cy_ota_callback_results_t ota_callback(cy_ota_cb_struct_t *cb_data);

/* OTA context */
cy_ota_context_ptr ota_context;
cy_stc_sd_host_context_t sdhc_host_context;
mtb_hal_sdio_t sdio_instance;
/**
 * @brief HTTP Credentials for OTA
 */
cy_awsport_ssl_credentials_t    http_credentials;

/**
 * @brief network parameters for OTA
 */
cy_ota_network_params_t     ota_network_params = { CY_OTA_CONNECTION_UNKNOWN };

/**
 * @brief aAgent parameters for OTA
 */
cy_ota_agent_params_t     ota_agent_params = { 0 };

/**
 * @brief Storage interface APIs for storage operations.
 */
cy_ota_storage_interface_t ota_interfaces =
{
   .ota_file_open            = cy_ota_storage_open,
   .ota_file_read            = cy_ota_storage_read,
   .ota_file_write           = cy_ota_storage_write,
   .ota_file_close           = cy_ota_storage_close,
   .ota_file_verify          = cy_ota_storage_verify,
   .ota_file_validate        = cy_ota_storage_image_validate,
   .ota_file_get_app_info    = cy_ota_storage_get_app_info
};

/////////////////////////////////////////////////////////////////////////////

static void on_connection_status(IotConnectConnectionStatus status) {
    // Add your own status handling
    switch (status) {
        case IOTC_CS_MQTT_CONNECTED:
            printf("IoTConnect Client Connected notification.\n");
            break;
        case IOTC_CS_MQTT_DISCONNECTED:
            printf("IoTConnect Client Disconnected notification.\n");
            break;
        default:
            printf("IoTConnect Client ERROR notification\n");
            break;
    }
}

static void on_ota(IotclC2dEventData data) {
    const char *ota_host = iotcl_c2d_get_ota_url_hostname(data, 0);
    if (ota_host == NULL) {
        printf("OTA host is invalid.\n");
        return;
    }
    const char *ota_path = iotcl_c2d_get_ota_url_resource(data, 0);
    if (ota_path == NULL) {
        printf("OTA resource is invalid.\n");
        return;
    }
    printf("OTA download request received for https://%s%s, but it is not implemented.\n", ota_host, ota_path);
}

// returns success on matching the expected format. Returns is_on, assuming "on" for true, "off" for false
static bool parse_on_off_command(const char* command, const char* name, bool *arg_parsing_success, bool *is_on, const char** message) {
    *arg_parsing_success = false;
    *message = NULL;
    size_t name_len = strlen(name);
    if (0 == strncmp(command, name, name_len)) {
        if (strlen(command) < name_len + 2) { // one for space and at least one character for the argument
            printf("ERROR: Expected command \"%s\" to have an argument\n", command);
            *message = "Command requires an argument";
            *arg_parsing_success = false;
        } else if (0 == strcmp(&command[name_len + 1], "on")) {
            *is_on = true;
            *message = "Value is now \"on\"";
            *arg_parsing_success = true;
        } else if (0 == strcmp(&command[name_len + 1], "off")) {
            *is_on = false;
            *message = "Value is now \"off\"";
            *arg_parsing_success = true;
        } else {
            *message = "Command argument";
            *arg_parsing_success = false;
        }
        // we matches the command
        return true;
    }

    // not our command
    return false;
}

static void on_command(IotclC2dEventData data) {
    const char * const BOARD_STATUS_LED = "board-user-led";
    const char * const DEMO_MODE_CMD = "demo-mode";
    const char * const SET_REPORTING_INTERVAL = "set-reporting-interval "; // with a space
	const char * const OTA_CMD = "ota-command";

    bool command_success = false;
    const char * message = NULL;

    const char *command = iotcl_c2d_get_command(data);
    const char *ack_id = iotcl_c2d_get_ack_id(data);

    if (command) {
        bool arg_parsing_success;
        printf("Command %s received with %s ACK ID\n", command, ack_id ? ack_id : "no");
        // could be a command without acknowledgment, so ackID can be null
        bool led_on;
		bool ota_on;

        if (parse_on_off_command(command, BOARD_STATUS_LED, &arg_parsing_success, &led_on, &message)) {
            command_success = arg_parsing_success;
            if (arg_parsing_success) {
                if (led_on) {
                    Cy_GPIO_Set(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN);
                } else {
                    Cy_GPIO_Clr(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN);
                }
            }
        } else if (parse_on_off_command(command, DEMO_MODE_CMD,  &arg_parsing_success, &is_demo_mode, &message)) {
            command_success = arg_parsing_success;
        } else if (0 == strncmp(SET_REPORTING_INTERVAL, command, strlen(SET_REPORTING_INTERVAL))) {
        	int value = atoi(&command[strlen(SET_REPORTING_INTERVAL)]);
        	if (0 == value) {
                message = "Argument parsing error";
        	} else {
        		reporting_interval = value;
        		printf("Reporting interval set to %d\n", value);
        		message = "Reporting interval set";
        		command_success =  true;
        	}
        } else if (parse_on_off_command(command, OTA_CMD,  &arg_parsing_success, &ota_on, &message)) {
            command_success = arg_parsing_success;
			//do OTA
			ota_in_progress = true;

			/* Initialize and start the OTA agent */
			printf("\n Initializing and starting the OTA agent.\n");
    		if(CY_RSLT_SUCCESS != initialize_ota()) {
        		printf("\n Initializing and starting the OTA agent failed.\n");
        		CY_ASSERT(0);
    		}
			
        } else {
            printf("Unknown command \"%s\"\n", command);
            message = "Unknown command";
        }
    } else {
        printf("Failed to parse command. Command or argument missing?\n");
        message = "Parsing error";
    }

    // could be a command without ack, so ack ID can be null
    // the user needs to enable acknowledgments in the template to get an ack ID
    if (ack_id) {
        iotcl_mqtt_send_cmd_ack(
                ack_id,
                command_success ? IOTCL_C2D_EVT_CMD_SUCCESS_WITH_ACK : IOTCL_C2D_EVT_CMD_FAILED,
                message // allowed to be null, but should not be null if failed, we'd hope
        );
    } else {
        // if we send an ack
        printf("Message status is %s. Message: %s\n", command_success ? "SUCCESS" : "FAILED", message ? message : "<none>");
    }
}

static cy_rslt_t publish_telemetry(void) {
    ipc_payload_t payload;
    // useful fro debugging - making sure we have te latest data:
    // printf("Has IPC Data: %s\n", cm33_ipc_has_received_message() ? "true" : "false");
    cm33_ipc_safe_get_and_clear_cached_detection(&payload);
    IotclMessageHandle msg = iotcl_telemetry_create();
    iotcl_telemetry_set_string(msg, "version", APP_VERSION);
    iotcl_telemetry_set_number(msg, "random", rand() % 100); // test some random numbers
    iotcl_telemetry_set_number(msg, "confidence", (int) (payload.confidence * 100.0f));
    iotcl_telemetry_set_number(msg, "class_id", payload.label_id);
    iotcl_telemetry_set_string(msg, "class", payload.label);
	iotcl_telemetry_set_bool(msg, "event_detected", payload.label_id > 0);

    iotcl_mqtt_send_telemetry(msg, false);
    iotcl_telemetry_destroy(msg);
    return CY_RSLT_SUCCESS;
}

void app_task(void *pvParameters) {
    (void) pvParameters;

//OTA
    // Disable watchdog to prevent wakeup from deep sleep mode
    Cy_WDT_Unlock();
    Cy_WDT_Disable();
    Cy_WDT_Lock();
    
    /* Initialize external flash */
	cy_rslt_t result;
    result = cy_ota_storage_init();

    /* External flash init failed. Stop program execution */
    if (result!=CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Validate all three images so that swap is permanent */
    for (uint32_t image_ID = 1; image_ID < 4; image_ID++)
    {
        cy_ota_storage_image_validate(image_ID);
    }
    
    // DO NOT print anything before we receive a message to avoice garbled output

    // we want to wait for CM33 to start receiving messages to prevent halts and errors below.
    while (!cm33_ipc_has_received_message()) {
        taskYIELD(); // wait for CM55
    }
    printf("App Task: CM55 IPC is ready. Resuming the application...\n");

    char iotc_duid[IOTCL_CONFIG_DUID_MAX_LEN] = IOTCONNECT_DUID;
    if (0 == strlen(iotc_duid)) {
        uint64_t hwuid = Cy_SysLib_GetUniqueId();
        uint32_t hwuidhi = (uint32_t)(hwuid >> 32);
        // not using low bytes in the name because they appear to be the same across all boards of the same type
        // feel free to modify the application to use these bytes
        // uint32_t hwuidlo = (uint32_t)(hwuid & 0xFFFFFFFF);
        sprintf(iotc_duid, IOTCONNECT_DUID_PREFIX"%08lx", (unsigned long) hwuidhi);
        printf("Generated device unique ID (DUID) is: %s\n", iotc_duid);
    }

    if (strlen(IOTCONNECT_DEVICE_CERT) == 0) {
		printf("ERROR: Device certificate is missing. Please configure the /IOTCONNECT credentials in app_config.h\n");
        goto exit_cleanup;
	}

    IotConnectClientConfig config;
    iotconnect_sdk_init_config(&config);
    config.connection_type = IOTCONNECT_CONNECTION_TYPE;
    config.cpid = IOTCONNECT_CPID;
    config.env =  IOTCONNECT_ENV;
    config.duid = iotc_duid;
    config.qos = 1;
    config.verbose = true;
    config.x509_config.device_cert = IOTCONNECT_DEVICE_CERT;
    config.x509_config.device_key = IOTCONNECT_DEVICE_KEY;
    config.callbacks.status_cb = on_connection_status;
    config.callbacks.cmd_cb = on_command;
    config.callbacks.ota_cb = on_ota;

    const char * conn_type_str = "(UNKNOWN)";
    if (config.connection_type == IOTC_CT_AWS) {
        conn_type_str = "AWS";
    } else if(config.connection_type == IOTC_CT_AZURE) {
        conn_type_str = "Azure";
    }

    printf("Current Settings:\n");
    printf("Platform: %s\n", conn_type_str);
    printf("DUID: %s\n", config.duid);
    printf("CPID: %s\n", config.cpid);
    printf("ENV: %s\n", config.env);

    // This will not return if it fails
    wifi_app_connect();

    iotc_mtb_time_obtain(IOTCONNECT_SNTP_SERVER);

    /* Initialize underlying support code that is needed for OTA and MQTT */
    if (cy_awsport_network_init() != CY_RSLT_SUCCESS)
    {
        printf("Initializing secure sockets Failed.\n");
        CY_ASSERT(0);
    }

			/* Initialize and start the OTA agent */
	printf("\n Initializing and starting the OTA agent.\n");
    if(CY_RSLT_SUCCESS != initialize_ota()) {
        printf("\n Initializing and starting the OTA agent failed.\n");
        CY_ASSERT(0);
    }
	printf("TASK return...\n");
	return;

    cy_rslt_t ret = iotconnect_sdk_init(&config);
    if (CY_RSLT_SUCCESS != ret) {
        printf("Failed to initialize the IoTConnect SDK. Error code: %u\n", (unsigned int) ret);
        goto exit_cleanup;
    }

    for (int i = 0; i < 10; i++) {
        ret = iotconnect_sdk_connect();
        if (CY_RSLT_SUCCESS != ret) {
            printf("Failed to initialize the IoTConnect SDK. Error code: %u\n", (unsigned int) ret);
            goto exit_cleanup;
        }
        
        int max_messages = is_demo_mode ? 6000 : 300;
        for (int j = 0; iotconnect_sdk_is_connected() && j < max_messages; j++) {
			if(ota_in_progress) {
				printf("iotconnect break1\n");
				break;
			}
            cy_rslt_t result = publish_telemetry();
            if (result != CY_RSLT_SUCCESS) {
                break;
                }
            iotconnect_sdk_poll_inbound_mq(reporting_interval);
        }
        iotconnect_sdk_disconnect();
			if(ota_in_progress) {
				printf("iotconnect break2\n");
				break;
			}
    }
    iotconnect_sdk_deinit();

    printf("\nAppTask Done.\n");
    while (1) {
        taskYIELD();
    }
    return;

    exit_cleanup:
    printf("\nError encountered. AppTask Done.\n");
    while (1) {
        taskYIELD();
    }
}

/*******************************************************************************
 * Function Name: initialize_ota()
 *******************************************************************************
 * Summary:
 *  Initialize and start the OTA update
 *
 * Parameters:
 *  pointer to Application context
 *
 * Return:
 *  cy_rslt_t
 *
 *******************************************************************************/
cy_rslt_t initialize_ota(void)
{
    cy_rslt_t               result;

    memset(&ota_network_params, 0, sizeof(ota_network_params));
    memset(&ota_agent_params,   0, sizeof(ota_agent_params));



    /* Network Parameters */
    ota_network_params.initial_connection    = OTA_CONNECTION_TYPE;
    ota_network_params.use_get_job_flow      = OTA_UPDATE_FLOW;


    /* HTTP Connection values */
    ota_network_params.http.server.host_name = OTA_HTTP_SERVER;  

    ota_network_params.http.server.port = OTA_HTTP_SERVER_PORT;


    /* For HTTP we change the file we GET when using a job flow */
    if (ota_network_params.use_get_job_flow == CY_OTA_JOB_FLOW)
    {
        /* For HTTP server to get "job" file */
        ota_network_params.http.file = OTA_HTTP_JOB_FILE;
    }
    else
    {
        /* For HTTP server to get "data" file directly */
        ota_network_params.http.file = OTA_HTTP_DATA_FILE;
    }

    /* set up HTTP credentials - only used if start_TLS is set */
    memset(&ota_network_params.http.credentials, 0x00, sizeof(ota_network_params.http.credentials ));

    http_credentials.alpnprotos         = NULL;
    http_credentials.alpnprotoslen      = 0;
    http_credentials.sni_host_name      = OTA_HTTP_SERVER;
    http_credentials.sni_host_name_size = strlen(OTA_HTTP_SERVER) + 1;
	//http_credentials.sni_host_name      = S3_SNI;
	//http_credentials.sni_host_name_size	= strlen(http_credentials.sni_host_name) + 1;
    http_credentials.root_ca            = ROOT_CA_CERTIFICATE;
    http_credentials.root_ca_size       = strlen(http_credentials.root_ca) + 1;
/*
    http_credentials.client_cert        = CLIENT_CERTIFICATE;
    http_credentials.client_cert_size   = strlen(http_credentials.client_cert) + 1;
    http_credentials.private_key        = CLIENT_KEY;
    http_credentials.private_key_size   = strlen(http_credentials.private_key) + 1;
*/

    http_credentials.client_cert        = NULL;
    http_credentials.client_cert_size   = 0;
    http_credentials.private_key        = NULL;
    http_credentials.private_key_size   = 0;

    ota_network_params.http.credentials = http_credentials;

    /* OTA Agent parameters */
    ota_agent_params.validate_after_reboot  = 1;  /* 1 = validate the image after reboot */
    ota_agent_params.reboot_upon_completion = 1;  /* 1 = reboot when download is finished */
    ota_agent_params.cb_func                = ota_callback;
    ota_agent_params.cb_arg                 = &ota_context;
    ota_agent_params.do_not_send_result     = 1;  /* 1 = do not send result */

    result = cy_ota_agent_start(&ota_network_params, &ota_agent_params, &ota_interfaces, &ota_context);
    if (result != CY_RSLT_SUCCESS)
    {
        cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "cy_ota_agent_start() Failed - result: 0x%lx\n", result);
    }
    cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "cy_ota_agent_start() Result: 0x%lx\n", result);

    return result;
}

/*******************************************************************************
 * Function Name: ota_callback()
 *******************************************************************************
 * Summary:
 *  Got a callback from OTA
 *
 *  @param[in][out] cb_data - structure holding information for the Application
 *
 * Return:
 *  CY_OTA_CB_RSLT_OTA_CONTINUE - OTA Agent to continue with function, using modified data from Application
 *  CY_OTA_CB_RSLT_OTA_STOP     - OTA Agent to End current update session (do not quit).
 *  CY_OTA_CB_RSLT_APP_SUCCESS  - Application completed task, OTA Agent use success
 *  CY_OTA_CB_RSLT_APP_FAILED   - Application completed task, OTA Agent use failure
 *
 *******************************************************************************/
cy_ota_callback_results_t ota_callback(cy_ota_cb_struct_t *cb_data)
{
    cy_ota_callback_results_t   cb_result = CY_OTA_CB_RSLT_OTA_CONTINUE;    /* OTA Agent to continue as normal */
    const char                  *state_string;
    const char                  *error_string;
    const char                  *reason_string;


    if (cb_data == NULL)
    {
        return CY_OTA_CB_RSLT_OTA_STOP;
    }

    state_string  = cy_ota_get_state_string(cb_data->ota_agt_state);
    error_string  = cy_ota_get_error_string(cy_ota_get_last_error());
    reason_string = cy_ota_get_callback_reason_string(cb_data->reason);

    /* Normal OTA callbacks here */
    cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "APP OTA CB NORMAL: state:%d  %s reason:%d %s\n",
                cb_data->ota_agt_state, state_string, cb_data->reason, reason_string);

    switch (cb_data->reason)
    {

    case CY_OTA_LAST_REASON:
        break;

    case CY_OTA_REASON_SUCCESS:
        cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB SUCCESS state:%d %s last_error:%s\n", cb_data->ota_agt_state, state_string, error_string);
        break;
    case CY_OTA_REASON_FAILURE:
        cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB FAILURE state:%d %s last_error:%s\n", cb_data->ota_agt_state, state_string, error_string);
        break;
    case CY_OTA_REASON_STATE_CHANGE:

        switch (cb_data->ota_agt_state)
        {
            /* informational */
        case CY_OTA_STATE_NOT_INITIALIZED:
        case CY_OTA_STATE_EXITING:
        case CY_OTA_STATE_INITIALIZING:
        case CY_OTA_STATE_AGENT_STARTED:
        case CY_OTA_STATE_AGENT_WAITING:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB STATE CHANGE state:%d %s last_error:%s\n", cb_data->ota_agt_state, state_string, error_string);
            break;

        case CY_OTA_STATE_START_UPDATE:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB STATE CHANGE CY_OTA_STATE_START_UPDATE\n");
            break;

        case CY_OTA_STATE_STORAGE_OPEN:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB STORAGE OPEN\n");
            break;

        case CY_OTA_STATE_STORAGE_WRITE:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_NOTICE, "APP OTA CB STORAGE WRITE %ld%% (%ld of %ld)\n", cb_data->percentage, cb_data->bytes_written, cb_data->total_size);
            break;

        case CY_OTA_STATE_STORAGE_CLOSE:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB STORAGE CLOSE\n");
            break;

        case CY_OTA_STATE_JOB_CONNECT:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB CONNECT FOR JOB\n");
            /* NOTE:
             *  HTTP - json_doc holds the HTTP "GET" request
             */

            if ( (cb_data->connection_type == CY_OTA_CONNECTION_HTTP) || (cb_data->connection_type == CY_OTA_CONNECTION_HTTPS) )
            {
                if ((cb_data->broker_server.host_name == NULL) ||
                    ( cb_data->broker_server.port == 0) ||
                    ( strlen(cb_data->file) == 0) )
                {
                    cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "ERROR in callback data: HTTP: server: %p port: %d topic: '%p'\n",
                            cb_data->broker_server.host_name, cb_data->broker_server.port, cb_data->file);
                    return CY_OTA_CB_RSLT_OTA_STOP;
                }
                cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "HTTP: server:%s port: %d file: '%s'\n",
                        cb_data->broker_server.host_name, cb_data->broker_server.port, cb_data->file);
            }


            break;
        case CY_OTA_STATE_JOB_DOWNLOAD:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB JOB DOWNLOAD using:\n");
            /* NOTE:

             *  HTTP - json_doc holds the HTTP "GET" request
             */

                if ( (cb_data->connection_type == CY_OTA_CONNECTION_HTTP) || (cb_data->connection_type == CY_OTA_CONNECTION_HTTPS) )
            {
                cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "HTTP: '%s'\n", cb_data->file);
            }
            break;

        case CY_OTA_STATE_JOB_DISCONNECT:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB JOB DISCONNECT\n");

            break;

        case CY_OTA_STATE_JOB_PARSE:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, " APP OTA CB PARSE JOB\n");

            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "      '%.*s' \n", strlen(cb_data->json_doc), cb_data->json_doc);

            break;

        case CY_OTA_STATE_JOB_REDIRECT:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "TODO: APP OTA CB JOB REDIRECT\n");
            break;

        case CY_OTA_STATE_DATA_CONNECT:

            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB CONNECT FOR DATA\n");

            break;

        case CY_OTA_STATE_DATA_DOWNLOAD:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "APP OTA CB DATA DOWNLOAD using:\n");
            /* NOTE:

             *  HTTP - json_doc holds the HTTP "GET" request
             */
if ( (cb_data->connection_type == CY_OTA_CONNECTION_HTTP) || (cb_data->connection_type == CY_OTA_CONNECTION_HTTPS) )
            
            {

                cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "HTTP: '%s' \n", cb_data->json_doc);
                cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "                        file: '%s' \n", cb_data->file);
                }
            break;

        case CY_OTA_STATE_DATA_DISCONNECT:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB DATA DISCONNECT\n");

            break;

        case CY_OTA_STATE_VERIFY:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB VERIFY\n");

            break;

        case CY_OTA_STATE_RESULT_REDIRECT:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB RESULT REDIRECT\n");

            break;

        case CY_OTA_STATE_RESULT_CONNECT:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB SEND RESULT CONNECT using ");
            /* NOTE:

             *  HTTP - json_doc holds the HTTP "GET" request
             */

            if ( (cb_data->connection_type == CY_OTA_CONNECTION_HTTP) || (cb_data->connection_type == CY_OTA_CONNECTION_HTTPS) )
            {
                cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "HTTP: Server:%s port: %d\n", cb_data->broker_server.host_name, cb_data->broker_server.port);
            }

            break;

        case CY_OTA_STATE_RESULT_SEND:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB SENDING RESULT using ");
            /* NOTE:

             *  HTTP - json_doc holds the HTTP "PUT"
             */

            if ( (cb_data->connection_type == CY_OTA_CONNECTION_HTTP) || (cb_data->connection_type == CY_OTA_CONNECTION_HTTPS) )

            {
                cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "HTTP: '%s' \n", cb_data->json_doc);
            }

            break;

        case CY_OTA_STATE_RESULT_RESPONSE:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB Got Result response\n");

            break;

        case CY_OTA_STATE_RESULT_DISCONNECT:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB Result Disconnect\n");

            break;

        case CY_OTA_STATE_OTA_COMPLETE:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB Session Complete\n");
            break;

        case CY_OTA_NUM_STATES:
            break;

        default:
            break;

        }   /* switch state */
        break;
    }

    return cb_result;
}

/* [] END OF FILE */
