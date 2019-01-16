/*
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#include "iothub_c_client.h"

/* Paste in the your iothub connection string  */
static const char* connectionString = "HostName=iothubks.azure-devices.net;DeviceId=cmac;SharedAccessKey=ti6bdDIjsjNaMn4i263ZPKhMue7b8iCZBmxx12OfUnI=";

#define MESSAGE_COUNT        1
static bool g_continueRunning = true;
static size_t g_message_count_send_confirmations = 0;
static const char* proxy_host = NULL; // "Web proxy name here"
static int proxy_port = 0;               // Proxy port
static const char* proxy_username = NULL; // Proxy user name
static const char* proxy_password = NULL; // Proxy password

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)userContextCallback;
    // When a message is sent this callback will get envoked
    g_message_count_send_confirmations++;
    (void)printf("Confirmation callback received for message %lu with result %s\r\n", (unsigned long)g_message_count_send_confirmations, ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    (void)reason;
    (void)user_context;
    // This sample DOES NOT take into consideration network outages.
    if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) {
        (void)printf("The device client is connected to iothub\r\n");
    } else {
        (void)printf("The device client has been disconnected\r\n");
    }
}

int main(void)
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    IOTHUB_MESSAGE_HANDLE message_handle;
    size_t messages_sent = 0;
    const char* telemetry_msg = "test_message";

    // Select the Protocol to use with the connection
#ifdef SAMPLE_MQTT
    protocol = MQTT_Protocol;
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
    protocol = MQTT_WebSocket_Protocol;
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
    protocol = AMQP_Protocol;
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
    protocol = AMQP_Protocol_over_WebSocketsTls;
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
    protocol = HTTP_Protocol;
#endif // SAMPLE_HTTP

    // Used to initialize IoTHub SDK subsystem
    (void)IoTHub_Init();

    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle;

    (void)printf("Creating IoTHub Device handle\r\n");
    // Create the iothub handle here
    device_ll_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, protocol);
    if (device_ll_handle == NULL) {
        (void)printf("Failure createing Iothub device.  Hint: Check you connection string.\r\n");
        return -1;
    }

    // Set any option that are neccessary.
    // For available options please see the iothub_sdk_options.md documentation
    bool traceOn = true;
    IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_LOG_TRACE, &traceOn);

    // Set proxy informaiton
    if (proxy_host) {
      HTTP_PROXY_OPTIONS http_proxy_options = { 0 };
      http_proxy_options.host_address = proxy_host;
      http_proxy_options.port = proxy_port;
      http_proxy_options.username = proxy_username;
      http_proxy_options.password = proxy_password;

      if (IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_HTTP_PROXY, &http_proxy_options) != IOTHUB_CLIENT_OK) {
        (void)printf("failure to set proxy\n");
      }
    }

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    // Setting the Trusted Certificate.  This is only necessary on system with without
    // built in certificate stores.
    IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#if defined SAMPLE_MQTT || defined SAMPLE_MQTT_WS
    //Setting the auto URL Encoder (recommended for MQTT). Please use this option unless
    //you are URL Encoding inputs yourself.
    //ONLY valid for use with MQTT
    //bool urlEncodeOn = true;
    //IoTHubDeviceClient_LL_SetOption(iothub_ll_handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);
#endif

    // Setting connection status callback to get indication of connection to iothub
    (void)IoTHubDeviceClient_LL_SetConnectionStatusCallback(device_ll_handle, connection_status_callback, NULL);

    while (g_continueRunning) {
        // Construct the iothub message from a string or a byte array
        message_handle = IoTHubMessage_CreateFromString(telemetry_msg);

        (void)printf("Sending message %d to IoTHub\r\n", (int)(messages_sent + 1));
        IoTHubDeviceClient_LL_SendEventAsync(device_ll_handle, message_handle, send_confirm_callback, NULL);

        // The message is copied to the sdk so the we can destroy it
        IoTHubMessage_Destroy(message_handle);

        messages_sent++;

        if (g_message_count_send_confirmations >= MESSAGE_COUNT)
        {
            g_continueRunning = false;
        }

        IoTHubDeviceClient_LL_DoWork(device_ll_handle);
        ThreadAPI_Sleep(500);
    }

    // Clean up the iothub sdk handle
    IoTHubDeviceClient_LL_Destroy(device_ll_handle);
    // Free all the sdk subsystem
    IoTHub_Deinit();

    printf("Press any key to continue");
    (void)getchar();

    return 0;
}
