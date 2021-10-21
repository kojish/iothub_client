/*
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
#include "iothub_c_client.h"
#include <time.h>

/* Paste in the your iothub connection string  */
static const char* connectionString = "HostName=iothubks.azure-devices.net;DeviceId=c_client;SharedAccessKey=o1Yl0B/KmICoxSWEdqc2RAvLLspCDoJN4XSdCN8oKs8=";

// Path to the Edge "owner" root CA certificate
static const char* edge_ca_cert_path = "/home/workdir/certs/azure-iot-test-only.root.ca.cert.pem";

#define MESSAGE_COUNT        1
static bool g_continueRunning = true;
static size_t g_message_count_send_confirmations = 0;
static int g_interval = 1000;
/* Web proxy information */
static const char* proxy_host = NULL; // "Web proxy name here"
static int proxy_port = 0;               // Proxy port
static const char* proxy_username = NULL; // Proxy user name
static const char* proxy_password = NULL; // Proxy password

/*
 * Called to receive a delivery confirmation of the IoT Hub message.
 */
static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback) {

    switch(result) {
    case IOTHUB_CLIENT_CONFIRMATION_OK:
        printf("Message sent to IoT Hub is confirmed [%lu] with result %s, %s\r\n", 
            (unsigned long)++g_message_count_send_confirmations,
            MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result),
            (char *)userContextCallback);
        break;
    case IOTHUB_CLIENT_CONFIRMATION_BECAUSE_DESTROY:
        printf("Message is destroyed [%lu] with result %s, %s\r\n", 
            (unsigned long)++g_message_count_send_confirmations,
            MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result),
            (char *)userContextCallback);
        break;
    case IOTHUB_CLIENT_CONFIRMATION_MESSAGE_TIMEOUT:
        printf("Message confirmation timeout occured [%lu] with result %s, %s\r\n", 
            (unsigned long)++g_message_count_send_confirmations,
            MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result),
            (char *)userContextCallback);
        break;
    case IOTHUB_CLIENT_CONFIRMATION_ERROR:
        printf("Message confirmation error [%lu] with result %s, %s\r\n", 
            (unsigned long)++g_message_count_send_confirmations,
            MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result),
            (char *)userContextCallback);
        break;
    DEFAULT:
        printf("Unknown confirmation result.\n");
        break;
    }
}

/*
 * Called to receive updates of the connection to IoT Hub.
 */
static void connection_status(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context) {

    if (result != IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) {
        printf("The device client has been disconnected\r\n");
    }

    // The detail connection information 
    switch(reason) {
    case IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN:
        printf("SAS token expired.\n");
        break;
    case IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED:
        printf("Device is disabled.\n");
        break;
    case IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL:
        printf("Bad credential.\n");
        break;
    case IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED: 
        printf("Retry is expired.\n");
        break;
    case IOTHUB_CLIENT_CONNECTION_NO_NETWORK:
        printf("No network connection.\n");
        break;
    case IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR: 
        printf("Communication error occured.\n");
        break;
    case IOTHUB_CLIENT_CONNECTION_OK:
        printf("Connection is successfully created with IoT Hub.\n");
        break;
    case IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE:
        printf("No ping response.\n");
        break;
    default:
        printf("Unknown connection status received.\n");
        break;
    }
}


/*
 * Called when C2D message is received from IoT Hub.
 */
static IOTHUBMESSAGE_DISPOSITION_RESULT c2d_message(IOTHUB_MESSAGE_HANDLE message, void* user_context) {
    (void)user_context;
    const char* messageId;
    const char* correlationId;

    // Message properties
    if ((messageId = IoTHubMessage_GetMessageId(message)) == NULL) {
        messageId = "<unavailable>";
    }

    if ((correlationId = IoTHubMessage_GetCorrelationId(message)) == NULL) {
        correlationId = "<unavailable>";
    }

    IOTHUBMESSAGE_CONTENT_TYPE content_type = IoTHubMessage_GetContentType(message);
    if (content_type == IOTHUBMESSAGE_BYTEARRAY) {
        const unsigned char* buff_msg;
        size_t buff_len;

        if (IoTHubMessage_GetByteArray(message, &buff_msg, &buff_len) != IOTHUB_MESSAGE_OK) {
            printf("Failure retrieving byte array message\r\n");
        } else {
            printf("Received Binary message\r\nMessage ID: %s\r\n Correlation ID: %s\r\n Data: <<<%.*s>>> & Size=%d\r\n",
                    messageId,
                    correlationId,
                    (int)buff_len,
                    buff_msg,
                    (int)buff_len);
        }
    } else {
        const char* string_msg = IoTHubMessage_GetString(message);
        if (string_msg == NULL) {
            printf("Failure retrieving byte array message\r\n");
        } else {
            printf("Received String Message\r\nMessage ID: %s\r\n Correlation ID: %s\r\n Data: <<<%s>>>\r\n",
                    messageId,
                    correlationId,
                    string_msg);
        }
    }

    const char* property_value = "property_value";
    const char* property_key = IoTHubMessage_GetProperty(message, property_value);
    if (property_key != NULL) {
        printf("\r\nMessage Properties:\r\n");
        printf("\tKey: %s Value: %s\r\n", property_value, property_key);
    }

    return IOTHUBMESSAGE_ACCEPTED;
}

/*
 * This is called if the device method is invoked.
 * If you call the method from portal, set "SetInterval" for method name, and interval (msec) for payload. Note that the interval must be 1000 or greater.
 */
static int device_method(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback) {
    const char* method = "SetInterval";
    int newInterval = 0;

    int status = 501;
    const char* RESPONSE_STRING = "{ \"Response\": \"Unknown method requested.\" }";

    printf("Device Method name:    %s\n", method_name);
    printf("Device Method payload: %d: %s\n", (int)size, (const char*)payload);
    printf("Current interval: %d, New Interval: %d\n", g_interval, newInterval);

    if (strcmp(method_name, method) == 0) {
        if (payload) {
            sscanf((const char *)payload, "%d", &newInterval);

            // Interval must be greater than 1000.
            if (newInterval > 999) {
                g_interval = newInterval;
                status = 200;
                RESPONSE_STRING = "{ \"Response\": \"Telemetry reporting interval updated.\" }";
            } else {
                status = 500;
                RESPONSE_STRING = "{ \"Response\": \"Invalid telemetry reporting interval.\" }";
            }
        }
    }

    printf("\r\nResponse status: %d\r\n", status);
    printf("New Interval: %d\r\n", newInterval);
    printf("Response payload: %s\r\n\r\n", RESPONSE_STRING);

    *resp_size = strlen(RESPONSE_STRING);
    if ((*response = malloc(*resp_size)) == NULL) {
        status = -1;
    } else {
        memcpy(*response, RESPONSE_STRING, *resp_size);
    }

    return status;
}

/*
 *  A callback function to be called.
 *  The argument 'status_code' is not documented at this point.
 *  status_code 204 indicates successfully updated, otherwise it will be 400.
 *  Reference: https://github.com/Azure/azure-iot-sdk-c/issues/1351
 */
static void reported_state(int status_code, void* userContextCallback) {

    printf("Device Twin reported properties update completed with result: %d\r\n", status_code);
}

static unsigned char device_twin_data[256];
static const char report_format[] = "{\"temperature\":%d, \"lastOilChangeData\": \"%s\"}";
/*
 *  A callback function to be called when desired property in device twin is updated.
 */
static void device_twin_callback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback) {

    char reported_data[256];

    printf("Device Twin (Desired) updated: %s\n", payLoad);
    snprintf(reported_data, sizeof(reported_data), report_format, 35, "2021");
    printf("REPORT_DATA %s\n", reported_data);

    // Sending the report from device.
    if(IoTHubDeviceClient_LL_SendReportedState((IOTHUB_DEVICE_CLIENT_LL_HANDLE)userContextCallback, 
                                               (const unsigned char*)reported_data,
                                               strlen(reported_data),
                                               reported_state,
                                               NULL) != IOTHUB_CLIENT_OK) {
        printf("Failed to set device method callback\n");
    }
}

/**
    Read the certificate file and provide a null terminated string
    containing the certificate.
*/
static char *obtain_edge_ca_certificate(void)
{
    char *result = NULL;
    FILE *ca_file = NULL;

    ca_file = fopen(edge_ca_cert_path, "r");
    if (ca_file == NULL) {
        printf("Cert file could not open for reading %s\r\n", edge_ca_cert_path);
    } else {
        size_t file_size;

        fseek(ca_file, 0, SEEK_END);
        file_size = ftell(ca_file);
        fseek(ca_file, 0, SEEK_SET);
        // increment size to hold the null term
        file_size += 1;

        if (file_size == 0) { // check wrap around
            printf("File size invalid for %s\r\n", edge_ca_cert_path);
        } else {
            result = (char*)calloc(file_size, 1);
            if (result == NULL) {
                printf("Could not allocate memory to hold the certificate\r\n");
            } else {
                // copy the file into the buffer
                size_t read_size = fread(result, 1, file_size - 1, ca_file);
                if (read_size != file_size - 1) {
                    printf("Error reading file %s\r\n", edge_ca_cert_path);
                    free(result);
                    result = NULL;
                }
            }
        }
        fclose(ca_file);
    }

    return result;
}

/*
 * main function. No argument is needed to start.
 */
int main(int argc, char **argv) {

    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    IOTHUB_MESSAGE_HANDLE message_handle;
    char deviceId[8] = "cmac";
    int pressure;
    int rotation;
    int level;
    char msg_buf[256];
    time_t timer;
    struct tm *local;
    char evtime[64];
    char *cert_str = NULL;

    // Select the protocol to use with the connection
#ifdef USE_PROTOCOL_MQTT
    protocol = MQTT_Protocol;
#endif
#ifdef USE_PROTOCOL_MQTT_OVER_WEBSOCKETS
    protocol = MQTT_WebSocket_Protocol;
#endif
#ifdef USE_PROTOCOL_AMQP
    protocol = AMQP_Protocol;
#endif
#ifdef USE_PROTOCOL_AMQP_OVER_WEBSOCKETS
    protocol = AMQP_Protocol_over_WebSocketsTls;
#endif
#ifdef USE_PROTOCOL_HTTP
    protocol = HTTP_Protocol;
#endif

    // Used to initialize IoTHub SDK subsystem
    IoTHub_Init();

    IOTHUB_DEVICE_CLIENT_LL_HANDLE dev_handle;

    printf("Creating IoTHub Device handle\r\n");
    // Create the iothub handle here
    dev_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, protocol);
    if (dev_handle == NULL) {
        printf("Failure createing Iothub device.  Hint: Check you connection string.\r\n");
        return -1;
    }

    // Set any option that are neccessary.
    // For available options please see the iothub_sdk_options.md documentation
    bool traceOn = false;
    IoTHubDeviceClient_LL_SetOption(dev_handle, OPTION_LOG_TRACE, &traceOn);

    // Set proxy informaiton
    if (proxy_host) {
      HTTP_PROXY_OPTIONS http_proxy_options = { 0 };
      http_proxy_options.host_address = proxy_host;
      http_proxy_options.port = proxy_port;
      http_proxy_options.username = proxy_username;
      http_proxy_options.password = proxy_password;

      if (IoTHubDeviceClient_LL_SetOption(dev_handle, OPTION_HTTP_PROXY, &http_proxy_options) != IOTHUB_CLIENT_OK) {
        printf("failure to set proxy\n");
      }
    }

    // This is for reading the cretification file.
/*
    cert_str = obtain_edge_ca_certificate();
    if(cert_str != NULL) {
        IoTHubDeviceClient_LL_SetOption(dev_handle, OPTION_TRUSTED_CERT, cert_str);
    }
*/

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    // Setting the Trusted Certificate.
    IoTHubDeviceClient_LL_SetOption(dev_handle, OPTION_TRUSTED_CERT, certificates);
#endif

#if defined USE_PROTOCOL_MQTT || defined USE_PROTOCOL_MQTT_WS
    // Setting the auto URL Encoder (recommended for MQTT). Please use this option unless you are URL Encoding inputs yourself.
    // Note that this is ONLY valid for use with MQTT
    bool urlEncodeOn = true;
    IoTHubDeviceClient_LL_SetOption(dev_handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);
#endif
    
    // Setting C2D message callback to receive a message from IoT Hub.
    if(IoTHubDeviceClient_LL_SetMessageCallback(dev_handle, c2d_message, NULL) != IOTHUB_CLIENT_OK) {
      printf("Failed to set C2D message callback\n");
      return -1;
    }
    // Setting connection status callback to get indication of connection to iothub
    if(IoTHubDeviceClient_LL_SetConnectionStatusCallback(dev_handle, connection_status, NULL) != IOTHUB_CLIENT_OK) {
      printf("Failed to set connection status callback\n");
      return -1;
    }
    // Setting device method callback
    if(IoTHubDeviceClient_LL_SetDeviceMethodCallback(dev_handle, device_method, NULL) != IOTHUB_CLIENT_OK) {
      printf("Failed to set device method callback\n");
      return -1;
    }
    // Setting device method callback
    if(IoTHubDeviceClient_LL_SetDeviceTwinCallback(dev_handle, device_twin_callback, dev_handle) != IOTHUB_CLIENT_OK) {
      printf("Failed to set device method callback\n");
      return -1;
    }

    while (g_continueRunning) {

        // Get local time at device side
        timer = time(NULL);
        local = localtime(&timer);
        memset(evtime, 0, sizeof(evtime));
        sprintf(evtime, "%4d-%02d-%02d %02d:%02d:%02d", 
                local->tm_year + 1900,
                local->tm_mon+1,
                local->tm_mday,
                local->tm_hour,
                local->tm_min,
                local->tm_sec);

        // Create an iothub message with random sample data
        pressure = 20 + ((int)rand()) % 15;
        rotation = 50 + ((int)rand()) % 15;
        level = 35 + ((int)rand()) % 15;
        memset(msg_buf, 0, sizeof(msg_buf));
        sprintf(msg_buf, "{\"deviceId\":\"%s\",\"pressure\":%d,\"rotation\":%d,\"level\":%d,\"event_time\":\"%s\"}",
                deviceId,
                pressure,
                rotation,
                level,
                evtime);

        message_handle = IoTHubMessage_CreateFromString(msg_buf);
        if(message_handle == NULL) {
            printf("Failed to create iot hub message from string\n");
            break;
        }

        // Send the message to iot hub
        IoTHubDeviceClient_LL_SendEventAsync(dev_handle, message_handle, send_confirm_callback, msg_buf);
        // Delete the message handle as the internal copy is created and handled properly by sdk.
        IoTHubMessage_Destroy(message_handle);
        IoTHubDeviceClient_LL_DoWork(dev_handle);

        ThreadAPI_Sleep(g_interval);
    }

    // Clean up the iothub sdk handle
    IoTHubDeviceClient_LL_Destroy(dev_handle);
    // Free all the sdk subsystem
    IoTHub_Deinit();

    return 0;
}
