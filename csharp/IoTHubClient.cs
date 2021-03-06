using System;
using System.Text;
using System.Threading;
using Microsoft.Azure.Devices.Client;
using Newtonsoft.Json;
using System.IO;
using System.Threading.Tasks;
using Microsoft.Azure.Devices.Shared;
using Microsoft.Azure.Devices.Client.Transport.Mqtt;
using System.Net;
using System.Web.Profile;
using Microsoft.Azure.Devices.Provisioning.Client;
using Microsoft.Azure.Devices.Provisioning.Client.Transport;

namespace IoTHubClient
{
    class IoTHubClientSample
    {
        // HTTP proxy settings
        static string proxyUri = "";
        static int proxyPort = 443;
        static string proxyUser = "";
        static string proxyPassword = "";
        // デバイスIDとホスト名はご使用の環境に合わせて変更ください：
        static DeviceClient deviceClient;
        static string hubName = "";
        static string deviceId = "";    // デバイス名
        static string sharedAccessKey = "";
        static string connectionString = "HostName=iothubkssea.azure-devices.net;DeviceId=mypc;SharedAccessKey=fuPvLGhf0kxzK/woYCCZEOkKXy57bdS0Y6OW9kJVmoU=";
        // For DPS
        static string globalEndpoint = "global.azure-devices-provisioning.net";
        static string scopeId = "0ne0006E3BA";
        static string registrationId = "185a9487-6605-47b9-a630-f5a46f856b80";
        static string primaryKey = "D8+xk9ETzqhplZJ7EoMrKbEp/70bLiIktUKD1gG/1Nc=";
        static string secondaryKey = "D8+xk9ETzqhplZJ7EoMrKbEp/70bLiIktUKD1gG/1Nc=";
        static DeviceRegistrationResult registrationResult = null;

        // Proxy settings
        private static WebProxy getProxySettings(string proxyUri, int port, string user, string password)
        {
            if (proxyUri.Length < 1) return null;

            var uri = new Uri(string.Format("{0}:{1}", proxyUri, port));
            ICredentials credentials = new NetworkCredential(user, password);
            return new WebProxy(uri, true, null, credentials);

        }

        static void Main(string[] args)
        {
            var settings = new ITransportSettings[]
            {
                new MqttTransportSettings(Microsoft.Azure.Devices.Client.TransportType.Mqtt_WebSocket_Only)
                {
                    Proxy = getProxySettings(proxyUri, proxyPort, proxyUser, proxyPassword),
                }
            };

            provisionDevice(scopeId, registrationId, primaryKey, secondaryKey).Wait();

            if (registrationResult != null) {
                connectionString = "HostName=" + registrationResult.AssignedHub + ";DeviceId=" + registrationResult.DeviceId + ";SharedAccessKey=" + primaryKey;
            }
            Console.WriteLine(connectionString);

            deviceClient = DeviceClient.CreateFromConnectionString(connectionString, settings);
            deviceClient.SetConnectionStatusChangesHandler(ConnectionStatusChangeHandler);
            sendMessages();
            //sendDataFromFile();
            receiveCloud2DeviceMessage();
            remoteMethod();
            deviceTwin();
            Console.ReadLine();
        }

        private static async Task provisionDevice(string scope_id, string registrationId, string primaryKey, string secondaryKey)
        {
            var security = new SecurityProviderSymmetricKey(registrationId, primaryKey, secondaryKey);
            using (var transport = new ProvisioningTransportHandlerMqtt(TransportFallbackType.WebSocketOnly))
            {
                Console.WriteLine("ProvisioningClient strt");
                ProvisioningDeviceClient provClient = ProvisioningDeviceClient.Create(globalEndpoint, scope_id, security, transport);
                if (provClient == null) Console.Write("null 0");
                Console.WriteLine("ProvisioningClient strt1: " + provClient.ProductInfo);
               registrationResult = await provClient.RegisterAsync();
                if (registrationResult == null) Console.WriteLine("null;;;");
                else Console.WriteLine("ProvisioningClient strt2: " + registrationResult.Status);
                if (registrationResult.Status == ProvisioningRegistrationStatusType.Assigned)
                {
                    Console.WriteLine("ProvisioningClient strt3: " + registrationResult.Status);
                    Console.WriteLine($"ProvisioningClient AssignedHub: {registrationResult.AssignedHub}; DeviceID: {registrationResult.DeviceId}");
                }
            }
        }

        // IoT Hub とのコネクションステータスを受信する処理
        private static void ConnectionStatusChangeHandler(ConnectionStatus status, ConnectionStatusChangeReason reason)
        {
            Console.WriteLine();
            Console.WriteLine("Connection Status Changed to {0}", status);
            Console.WriteLine("Connection Status Changed Reason is {0}", reason);
            Console.WriteLine();
        }


        private static async void sendDataFromFile()
        {
            string localFile = @"C:\tmp\TEST_CSV.CSV"; // 対象ファイル
            StreamReader reader = new StreamReader(localFile, System.Text.Encoding.GetEncoding("shift_jis"));

            while (!reader.EndOfStream)
            {
                string line = reader.ReadLine();
                string[] values = line.Split(',');

                var telemetryDataPoint = new
                {
                    seihin_no1 = values[0],
                    seihin_no2 = int.Parse(values[1]),
                    seihin_no3 = int.Parse(values[2]),
                    tokuisaki_buban = values[3],
                    hinmei = values[4],
                    rl_kubun = values[5],
                    color = values[6],
                    kubun = values[7],
                    yearmonth = values[8],
                    val = int.Parse(values[9])
                };
                var messageString = JsonConvert.SerializeObject(telemetryDataPoint);
                var message = new Message(Encoding.UTF8.GetBytes(messageString));

                await deviceClient.SendEventAsync(message);

                Console.WriteLine("{0} > Sending message: {1}", DateTime.Now, messageString);
                Thread.Sleep(3000);
            }
        }

        // IoTHubへファイルをアップロードする処理
        private static async void uploadFile()
        {
            string localFile = @"C:\Temp\customers.csv"; // 対象ファイル
            Console.WriteLine("Uploading file: {0}", localFile);
            var watch = System.Diagnostics.Stopwatch.StartNew();

            using (var sourceData = new FileStream(localFile, FileMode.Open))
            {
                await deviceClient.UploadToBlobAsync(Path.GetFileName(sourceData.Name), sourceData);
            }

            watch.Stop();
            Console.WriteLine("Time to upload file: {0}ms\n", watch.ElapsedMilliseconds);
        }

        private static async void deviceTwin()
        {
            await deviceClient.SetDesiredPropertyUpdateCallbackAsync(OnDesiredPropertyChanged, null).ConfigureAwait(false);
        }

        private static async Task OnDesiredPropertyChanged(TwinCollection desiredProperties, object userContext)
        {
            Console.WriteLine("\tDesired property changed:");
            Console.WriteLine($"\t{desiredProperties.ToJson()}");
            Console.WriteLine("\tSending current time as reported property");
            TwinCollection reportedProperties = new TwinCollection();
            reportedProperties["DateTimeLastDesiredPropertyChangeReceived"] = DateTime.Now;
            await deviceClient.UpdateReportedPropertiesAsync(reportedProperties).ConfigureAwait(false);
        }

        // IoTHubからメッセージを受信する処理
        // メッセージの送信は、Device Explorerの[Messages To Device]タブ -> [Device ID]で対象デバイスを選択し
        // [Message]にてクライアントに送信したいメッセージを入力後、[Send]ボタンを押下します。
        private static async void receiveCloud2DeviceMessage()
        {
            Console.WriteLine("\nReceiving cloud to device messages from service");
            while (true)
            {
                Microsoft.Azure.Devices.Client.Message receivedMessage = await deviceClient.ReceiveAsync();
                if (receivedMessage == null) continue;

                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine("Received message: {0}", Encoding.ASCII.GetString(receivedMessage.GetBytes()));
                Console.ResetColor();

                await deviceClient.CompleteAsync(receivedMessage);
            }
        }

        // IoTHubへのデータ送信処理
        // 1秒毎に[deviceId, pressure, rotation, oil_level, event_time]をJSONにて送信
        private static async void sendMessages()
        {
            while (true)
            {
                int rand1 = new Random((int)DateTime.Now.Ticks & 0x0000FFFF).Next(25, 30);
                Thread.Sleep(1000);
                int rand2 = new Random((int)DateTime.Now.Ticks & 0x0000FFFF).Next(15, 25);
                int rand3 = new Random((int)DateTime.Now.Ticks & 0x0000FFFF).Next(1, 10);
                int rand4 = new Random((int)DateTime.Now.Ticks & 0x0000FFFF).Next(30, 45);

                var telemetryDataPoint = new
                {
                    deviceId = registrationResult.DeviceId,
                    pressure = rand1,
                    rotation = rand2,
                    oil_level = rand3,
                    fan_mode = rand4,
                    event_time = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"),
                };
                var messageString = JsonConvert.SerializeObject(telemetryDataPoint);
                var message = new Microsoft.Azure.Devices.Client.Message(Encoding.UTF8.GetBytes(messageString));

                await deviceClient.SendEventAsync(message);

                Console.WriteLine("{0} > Sending message: {1}", DateTime.Now, messageString);
            }
        }

        // IoT Hub からのダイレクトメソッド使用時に呼び出されるメソッド
        private static async void remoteMethod()
        {
            await deviceClient.SetMethodHandlerAsync(nameof(WriteToConsole), WriteToConsole, null).ConfigureAwait(false);
            await deviceClient.SetMethodHandlerAsync(nameof(GetDeviceName), GetDeviceName, null).ConfigureAwait(false);
        }

        private static Task<MethodResponse> WriteToConsole(MethodRequest methodRequest, object userContext)

        {
            Console.WriteLine($"\t *** {nameof(WriteToConsole)} was called.");
            Console.WriteLine();
            Console.WriteLine("\t{0}", methodRequest.DataAsJson);
            Console.WriteLine();

            return Task.FromResult(new MethodResponse(new byte[0], 200));

        }

        private static Task<MethodResponse> GetDeviceName(MethodRequest methodRequest, object userContext)
        {
            Console.WriteLine($"\t *** {nameof(GetDeviceName)} was called.");

            MethodResponse retValue;
            string result = "Error: userContext is null.";
            result = "{\"name\":\"" + deviceId + "\"}";
            Console.WriteLine(result);
            retValue = new MethodResponse(Encoding.UTF8.GetBytes(result), 200);

            return Task.FromResult(retValue);
        }
    }
}
