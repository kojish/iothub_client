using System;
using System.Text;
using System.Threading;
using Microsoft.Azure.Devices.Client;
using Newtonsoft.Json;
using System.IO;
using System.Threading.Tasks;
using Microsoft.Azure.Devices.Shared;

namespace IoTHubClient
{
    class IoTHubClientSample
    {
        // デバイスIDとホスト名はご使用の環境に合わせて変更ください：
        static string deviceId = "CSharpDevice";    // デバイス名
        static DeviceClient deviceClient;
        // 接続文字列
        static string connectionString = "HostName=iothubkssea.azure-devices.net;DeviceId=mypc;SharedAccessKey=fuPvLGhf0kxzK/woYCCZEOkKXy57bdS0Y6OW9kJVmoU=";

        static void Main(string[] args)
        {
            deviceClient = DeviceClient.CreateFromConnectionString(connectionString,
                                                                   Microsoft.Azure.Devices.Client.TransportType.Mqtt);
            deviceClient.SetConnectionStatusChangesHandler(ConnectionStatusChangeHandler);
            //sendMessages();
            sendData();
            receiveCloud2DeviceMessage();
            remoteMethod();
            deviceTwin();
            Console.ReadLine();
        }

        // IoT Hub とのコネクションステータスを受信する処理
        private static void ConnectionStatusChangeHandler(ConnectionStatus status, ConnectionStatusChangeReason reason)
        {
            Console.WriteLine();
            Console.WriteLine("Connection Status Changed to {0}", status);
            Console.WriteLine("Connection Status Changed Reason is {0}", reason);
            Console.WriteLine();

        }

        // CSVファイルを読み込み、IoT Hub へデータを送信する処理
        private static async void sendData()
        {
            string localFile = @"C:\tmp\TEST_CSV.CSV"; // 対象ファイル
            StreamReader reader = new StreamReader(localFile);

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
                int rand1 = new Random((int)DateTime.Now.Ticks & 0x0000FFFF).Next(20, 50);
                Thread.Sleep(1000);
                int rand2 = new Random((int)DateTime.Now.Ticks & 0x0000FFFF).Next(15, 40);
                int rand3 = new Random((int)DateTime.Now.Ticks & 0x0000FFFF).Next(1, 10);

                var telemetryDataPoint = new
                {
                    deviceId = deviceId,
                    pressure = rand1,
                    rotation = rand2,
                    oil_level = rand3,
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
