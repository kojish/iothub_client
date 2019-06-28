/*
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
#include "iothub_c_client.h"
#include <time.h>

 /* Paste in the your iothub connection string  */
 //static const char* connectionString = "HostName=iothubks.azure-devices.net;DeviceId=cmac;SharedAccessKey=ti6bdDIjsjNaMn4i263ZPKhMue7b8iCZBmxx12OfUnI=";

static const char* connectionString = "HostName=iothubkssea.azure-devices.net;DeviceId=client;SharedAccessKey=GNaJ+4g6a6AQZqFsbr/ZwgnDCOnjjumht23lKPptnI=";

// Path to the Edge "owner" root CA certificate
///static const char* edge_ca_cert_path = "/home/kshimizu/proj/azure-iot-sdk-c/workdir/certs/azure-iot-test-only.root.ca.cert.pem";
static const char* edge_ca_cert_path = "C:\tmp\azure-iot-test-only.root.ca.cert.pem";

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
 * Called to receive confirmation of the delivery of the IoT Hub message.
 */
static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback) {
	(void)userContextCallback;
	//    (void)printf("Confirmation callback received for message %lu with result %s\r\n", (unsigned long)g_message_count_send_confirmations, ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

/*
 * Called to receive updates of the connection to IoT Hub.
 */
static void connection_status(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context) {
	(void)reason;
	(void)user_context;
	// This sample DOES NOT take into consideration network outages.
	if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) {
		(void)printf("The device client is connected to iothub\r\n");
	}
	else {
		(void)printf("The device client has been disconnected\r\n");
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
			(void)printf("Failure retrieving byte array message\r\n");
		}
		else {
			(void)printf("Received Binary message\r\nMessage ID: %s\r\n Correlation ID: %s\r\n Data: <<<%.*s>>> & Size=%d\r\n", messageId, correlationId, (int)buff_len, buff_msg, (int)buff_len);
		}
	}
	else {
		const char* string_msg = IoTHubMessage_GetString(message);
		if (string_msg == NULL) {
			(void)printf("Failure retrieving byte array message\r\n");
		}
		else {
			(void)printf("Received String Message\r\nMessage ID: %s\r\n Correlation ID: %s\r\n Data: <<<%s>>>\r\n", messageId, correlationId, string_msg);
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

static int device_method(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* userContextCallback)
{
	const char* SetTelemetryIntervalMethod = "SetInterval";
	const char* device_id = (const char*)userContextCallback;
	char* end = NULL;
	int newInterval;

	int status = 501;
	const char* RESPONSE_STRING = "{ \"Response\": \"Unknown method requested.\" }";

	(void)printf("\r\nDevice Method called for device %s\r\n", device_id);
	(void)printf("Device Method name:    %s\r\n", method_name);
	(void)printf("Device Method payload: %.*s\r\n", (int)size, (const char*)payload);

	if (strcmp(method_name, SetTelemetryIntervalMethod) == 0)
	{
		if (payload)
		{
			newInterval = (int)strtol((char*)payload, &end, 10);

			// Interval must be greater than zero.
			if (newInterval > 0)
			{
				// expect sec and covert to ms
				g_interval = 1000 * (int)strtol((char*)payload, &end, 10);
				status = 200;
				RESPONSE_STRING = "{ \"Response\": \"Telemetry reporting interval updated.\" }";
			}
			else
			{
				status = 500;
				RESPONSE_STRING = "{ \"Response\": \"Invalid telemetry reporting interval.\" }";
			}
		}
	}

	(void)printf("\r\nResponse status: %d\r\n", status);
	(void)printf("Response payload: %s\r\n\r\n", RESPONSE_STRING);

	*resp_size = strlen(RESPONSE_STRING);
	if ((*response = (unsigned char *)malloc(*resp_size)) == NULL)
	{
		status = -1;
	}
	else
	{
		memcpy(*response, RESPONSE_STRING, *resp_size);
	}

	return status;
}

/*
 *  A callback function to be called
 */
static void reported_state(int status_code, void* userContextCallback) {
	(void)userContextCallback;
	(void)printf("Device Twin reported properties update completed with result: %d\r\n", status_code);
}

/*
 *  A callback function to be called
 */
static void device_twin_callback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback) {
	(void)update_state;
	(void)size;

	printf("JSON: %s\n", payLoad);

}

/**
	Read the certificate file and provide a null terminated string
	containing the certificate.
*/
static char* obtain_edge_ca_certificate(void)
{
	char* result = NULL;
	FILE* ca_file;
	errno_t error;

	error = fopen_s(&ca_file, edge_ca_cert_path, "r");
	if (ca_file == NULL)
	{
		printf("Error could not open file for reading %s\r\n", edge_ca_cert_path);
	}
	else
	{
		size_t file_size;

		(void)fseek(ca_file, 0, SEEK_END);
		file_size = ftell(ca_file);
		(void)fseek(ca_file, 0, SEEK_SET);
		// increment size to hold the null term
		file_size += 1;

		if (file_size == 0) // check wrap around
		{
			printf("File size invalid for %s\r\n", edge_ca_cert_path);
		}
		else
		{
			result = (char*)calloc(file_size, 1);
			if (result == NULL)
			{
				printf("Could not allocate memory to hold the certificate\r\n");
			}
			else
			{
				// copy the file into the buffer
				size_t read_size = fread(result, 1, file_size - 1, ca_file);
				if (read_size != file_size - 1)
				{
					printf("Error reading file %s\r\n", edge_ca_cert_path);
					free(result);
					result = NULL;
				}
			}
		}
		(void)fclose(ca_file);
	}

	return result;
}

// Obtain event message
static char *createMessage() {

	char deviceId[12] = "c_client";
	int pressure;
	int rotation;
	int level;
	char *buf;
	size_t buf_size = 512;
	time_t timer;
	struct tm local;
	errno_t error;
	char evtime[64];
	
	// Create the iothub message
	pressure = 20 + ((int)rand()) % 15;
	rotation = 50 + ((int)rand()) % 15;
	level = 35 + ((int)rand()) % 15;
	timer = time(NULL);
	error = localtime_s(&local, &timer);
	memset(evtime, 0, sizeof(evtime));
	sprintf_s(evtime, "%4d-%02d-%02d %02d:%02d:%02d", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
	buf = (char *)malloc(buf_size);
	if (buf == NULL) {
		printf("malloc() failed. return.");
		return NULL;
	}
	memset(buf, 0, sizeof(buf));
	sprintf_s(buf, buf_size, "{\"deviceId\":\"%s\",\"pressure\":%d,\"rotation\":%d,\"level\":%d,\"event_time\":\"%s\"}", deviceId, pressure, rotation, level, evtime);

	return buf;
}

// Create message from file
static char *createMessageFromFile(char *path) {
	char* result = NULL;
	FILE* fp;
	errno_t error;

	error = fopen_s(&fp, path, "r");
	if (fp == NULL)
	{
		printf("Error could not open file for reading %s\r\n", path);
	}
	else
	{
		(void)fclose(fp);
	}
}

/*
 * main function. No argument is needed.
 */
int main(int argc, char** argv) {

	IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
	IOTHUB_MESSAGE_HANDLE message_handle;
	size_t messages_sent = 0;
	char *msg_buf = NULL;
	char* cert_str = NULL;

	// Select the Protocol to use with the connection
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
	(void)IoTHub_Init();

	IOTHUB_DEVICE_CLIENT_LL_HANDLE dev_handle;

	(void)printf("Creating IoTHub Device handle\r\n");
	// Create the iothub handle here
	dev_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, protocol);
	if (dev_handle == NULL) {
		(void)printf("Failure createing Iothub device.  Hint: Check you connection string.\r\n");
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
			(void)printf("failure to set proxy\n");
		}
	}

	cert_str = obtain_edge_ca_certificate();
	IoTHubDeviceClient_LL_SetOption(dev_handle, OPTION_TRUSTED_CERT, cert_str);

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
	if (IoTHubDeviceClient_LL_SetMessageCallback(dev_handle, c2d_message, NULL) != IOTHUB_CLIENT_OK) {
		(void)printf("Failed to set C2D message callback\n");
		return -1;
	}
	// Setting connection status callback to get indication of connection to iothub
	if (IoTHubDeviceClient_LL_SetConnectionStatusCallback(dev_handle, connection_status, NULL) != IOTHUB_CLIENT_OK) {
		(void)printf("Failed to set connection status callback\n");
		return -1;
	}
	// Setting device method callback
	if (IoTHubDeviceClient_LL_SetDeviceMethodCallback(dev_handle, device_method, NULL) != IOTHUB_CLIENT_OK) {
		(void)printf("Failed to set device method callback\n");
		return -1;
	}
	// Sending the report from device. This should be called in another place such as device_twin_callback to report the status.
//    if(IoTHubDeviceClient_LL_SendReportedState(dev_handle, (const unsigned char*)reported_prop, strlen(reported_prop), reported_state, NULL) != IOTHUB_CLIENT_OK) {
//      (void)printf("Failed to set device method callback\n");
//      return -1;
//    }
	// Setting device method callback
	if (IoTHubDeviceClient_LL_SetDeviceTwinCallback(dev_handle, device_twin_callback, NULL) != IOTHUB_CLIENT_OK) {
		(void)printf("Failed to set device method callback\n");
		return -1;
	}

FILE* fp;
errno_t err;
char buf[1024];
char msg[1024];
char* ret;
char seihin_no1[4]; memset(seihin_no1, 0, sizeof(seihin_no1));
int seihin_no2 = 0;
int seihin_no3 = 0;
char tokuisaki_buban[16]; memset(tokuisaki_buban, 0, sizeof(tokuisaki_buban));
char hinmei[32]; memset(hinmei, 0, sizeof(hinmei));
char rl_kubun[4]; memset(rl_kubun, 0, sizeof(rl_kubun));
char color[8]; memset(color, 0, sizeof(color));
char kubun[32]; memset(kubun, 0, sizeof(kubun));
char yearmonth[12]; memset(yearmonth, 0, sizeof(yearmonth));
int val = 0;

	err = fopen_s(&fp, "C:\\tmp\\TEST_CSV.CSV","r");
	if (fp == NULL) {
		printf("Error could not open csv file for reading.\r\n");
		return 0;
	}

	while (g_continueRunning) {

//		msg_buf = createMessage();
//		if (msg_buf == NULL) {
//			continue;
//		}
	
		memset(buf, 0, sizeof(buf));
		ret = fgets(buf, sizeof(buf), fp);
		if (ret == NULL) {
			printf("Reached to end of the file\n");
			break;
		}
		memset(seihin_no1, 0, sizeof(seihin_no1));
		memset(tokuisaki_buban, 0, sizeof(tokuisaki_buban));
		memset(hinmei, 0, sizeof(hinmei));
		memset(rl_kubun, 0, sizeof(rl_kubun));
		memset(color, 0, sizeof(color));
		memset(kubun, 0, sizeof(kubun));
		memset(yearmonth, 0, sizeof(yearmonth));
		int r = sscanf_s(buf, "%[^,],%d,%d,%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%d", 
			seihin_no1,(unsigned)_countof(seihin_no1),
			&seihin_no2, &seihin_no3,
			tokuisaki_buban,(unsigned)_countof(tokuisaki_buban), hinmei, (unsigned)_countof(hinmei),
			rl_kubun, (unsigned)_countof(rl_kubun), color, (unsigned)_countof(color), kubun, (unsigned)_countof(kubun), 
			yearmonth, (unsigned)_countof(yearmonth), &val);
		
		sprintf_s(msg, "{\"seihin_no1\":\"%s\",\"seihin_no2\":%d,\"seihin_no3\":%d,\"tokuisaki_buban\":\"%s\",\"hinmei\":\"%s\",\"rl_kubun\":\"%s\",\"color\":\"%s\",\"kubun\":\"%s\",\"yearmonth\":\"%s\",\"val\":%d}",
			seihin_no1, seihin_no2, seihin_no3, tokuisaki_buban, hinmei, rl_kubun, color, kubun, yearmonth, val);

		message_handle = IoTHubMessage_CreateFromString(msg);
		if (message_handle == NULL) {
			(void)printf("Failed to create iot hub message from string\n");
			break;
		}
		(void)printf("Sending message [%d] %s\r\n", (int)(messages_sent + 1), msg);
		IoTHubDeviceClient_LL_SendEventAsync(dev_handle, message_handle, send_confirm_callback, NULL);
		// The message is copied to the sdk so the we can destroy it
		IoTHubMessage_Destroy(message_handle);
		IoTHubDeviceClient_LL_DoWork(dev_handle);

		//free(msg_buf);
		messages_sent++;
		ThreadAPI_Sleep(g_interval);
	}

	if(fp != NULL) (void)fclose(fp);

	// Clean up the iothub sdk handle
	IoTHubDeviceClient_LL_Destroy(dev_handle);
	// Free all the sdk subsystem
	IoTHub_Deinit();

	printf("Press any key to continue");
	(void)getchar();

	return 0;
}
