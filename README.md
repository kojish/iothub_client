# iothub_c_client
This is a client application for Azure IoT Hub. The application uses the lower level APIs with an \_ll\_ infix that is contained in the C SDK. This is just for learning purpose or possibly to support a resource constrained device for my project in the future. The appilcation will support the following:
* D2C and C2D message
* Device Twin
* Direct Methods
* Upload file to Azure storage
* Authentication: SAS key, X509
* Device Provisioning Service
* Leaf device of IoT Edge; Supporting downstream device

## Build Prerequisites
You will need to set-up Azure IoT Hub C SDK in your development environment before compiling the application. See Microsoft's relevant web site to install and configure to get it working.

## Dependency
* TBD. Tested with macOS only for now.

## How to build
1. Download and copy this application to the sample folder in the Azure IoT Hub C SDK which is azure-iot-sdk-c/iothub_client/samples/. The directory should look like the following:  
   azure-iot-sdk-c/iothub_client/samples/iothub_c_client
2. Open azure-iot-sdk-c/iothub_client/samples/CMakeLists.txt. Add "add_sample_directory(iothub_c_client)" to CMakeLists.txt to be compiled.
3. Go to azure-iot-sdk-c/cmake directory, and run "cmake --build ." to build the application. You will find the executable file under azure-iot-sdk-c/cmake/iothub_client/samples/iothub_c_client directory. The file name is iothub_c_client.
4. You can also run the "cmake --build ." command under azure-iot-sdk-c/cmake/iothub_client/sample/iothub_c_client diredtory after the initial build.
