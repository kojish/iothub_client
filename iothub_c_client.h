// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_C_CLIENT_H
#define IOTHUB_C_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

    void iothub_client_sample_module_sender(void);

#include <stdio.h>
#include <stdlib.h>

#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

// The protocol you wish to use should be uncommented
#define USE_PROTOCOL_MQTT
//#define USE_PROTOCOL_MQTT_OVER_WEBSOCKETS
//#define USE_PROTOCOL_AMQP
//#define USE_PROTOCOL_AMQP_OVER_WEBSOCKETS
//#define USE_PROTOCOL_HTTP
#ifdef USE_PROTOCOL_MQTT
    #include "iothubtransportmqtt.h"
#endif // USE_PROTOCOL_MQTT
#ifdef USE_PROTOCOL_MQTT_OVER_WEBSOCKETS
    #include "iothubtransportmqtt_websockets.h"
#endif // USE_PROTOCOL_MQTT_OVER_WEBSOCKETS
#ifdef USE_PROTOCOL_AMQP
    #include "iothubtransportamqp.h"
#endif // USE_PROTOCOL_AMQP
#ifdef USE_PROTOCOL_AMQP_OVER_WEBSOCKETS
    #include "iothubtransportamqp_websockets.h"
#endif // USE_PROTOCOL_AMQP_OVER_WEBSOCKETS
#ifdef USE_PROTOCOL_HTTP
    #include "iothubtransporthttp.h"
#endif // USE_PROTOCOL_HTTP

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_C_CLIENT_H */
