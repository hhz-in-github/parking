#include <DHT.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include "secrets.h"

String apiKey = SECRET_WRITE_APIKEY;
const char* server = "api.thingspeak.com";

// 使用软件串口连接到 TJC Nextion 屏幕
SoftwareSerial TJC(D3, D4);  // 定义软件串口对象，D3 为 RX，D4 为 TX

#define DHTPIN D5
#define WATER_SENSOR_PIN D2
#define LIGHT_SENSOR_PIN A0
#define SENSOR_POWER_PIN D7 // 传感器电源控制引脚
DHT dht(DHTPIN, DHT11, 15);
WiFiClient client;

// 存储多组 WiFi 凭据在 secrets.h 文件中
const char* ssidList[] = {SECRET_WIFI_SSID_1, SECRET_WIFI_SSID_2,SECRET_WIFI_SSID_3,SECRET_WIFI_SSID_4};
const char* passList[] = {SECRET_WIFI_PASS_1, SECRET_WIFI_PASS_2,SECRET_WIFI_PASS_3,SECRET_WIFI_PASS_4};

unsigned long nowtime;

void setup() {
  Serial.begin(115200);
  TJC.begin(9600);  // 修改为与 TJC 屏幕适配的波特率
  delay(10);
  dht.begin();

  Serial.println();
  Serial.println("可用WiFi网络列表:");

  // 打印可用WiFi网络列表
  int numNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numNetworks; i++) {
    Serial.println(WiFi.SSID(i));
  }
  Serial.println();
  // 选择并连接WiFi
  connectToWiFi();
    nowtime = millis(); //获取当前已经运行的时间
}

void loop() {
  float humidity1 = dht.readHumidity();
  int temperature1 = dht.readTemperature();
  int waterLevel = readWaterSensor();
  int lightIntensity = readLightSensor();

  if (isnan(humidity1) || isnan(temperature1)) {
    Serial.println("从DHT传感器读取失败!");
    return;
  }

  if (client.connect(server, 80)) {
    // 构建发送到ThingSpeak的数据字符串
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(lightIntensity);
    postStr += "&field2=";
    postStr += String(temperature1);
    postStr += "&field3=";
    postStr += String(humidity1);
    postStr += "&field4=";
    postStr += String(waterLevel);
    postStr += "\r\n\r\n";

    // 发送HTTP POST请求到ThingSpeak
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    // 在串口上打印传感器数据和发送信息
    Serial.print("光强度: ");
    Serial.print(lightIntensity); 
    Serial.print(" Lux, 温度: ");
    Serial.print(temperature1);
    Serial.print(" 摄氏度, 湿度: ");
    Serial.print(humidity1);
    Serial.print("%, 水位: ");
    Serial.print(waterLevel);
    Serial.println(" mm, 发送到ThingSpeak");
  
  // 更新 Nextion 屏幕
 //   while (TJC.read() >= 0); //清空串口缓冲区
      char str[50];
      char str1[50];
      char str2[50];
      char str3[50];
      //用sprintf来格式化字符串，给t0的txt属性赋值
   int arr[5]={21,22,23,24,25};
   for(int i=0;i<5;i++)
   {
         sprintf(str1, "page0.t2.txt=\"%d\"\xff\xff\xff", arr[i]);
   TJC.print(str1);
 }

   int arr1[5]={44,41,40,42,45};
   for(int j=0;j<5;j++)
   {
         sprintf(str2, "page0.t4.txt=\"%d\"\xff\xff\xff", arr1[j]);
   TJC.print(str2);
 }

//TJC.print("温度为：%d",&i);
  
    sprintf(str, "page0.t6.txt=\"%d\"\xff\xff\xff", lightIntensity);
   TJC.print(str); 
//    sprintf(str1, "page0.t2.txt=\"%d\"\xff\xff\xff", temperature1);
//   TJC.print(str1);
 //   sprintf(str2, "page0.t4.txt=\"%d\"\xff\xff\xff", humidity1);
 //  TJC.print(str2);    

   sprintf(str3, "page0.t8.txt=\"%d\"\xff\xff\xff", 0);
   TJC.print(str3);
 
  }

   

 
 client.stop();
  Serial.println("等待中…");
  delay(1000); // ThingSpeak需要最小15秒的延迟

}




/*

封装函数部分

*/
// 读取水位传感器值
  int readWaterSensor() {
  int sensorValue = 0;

  pinMode(SENSOR_POWER_PIN, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN, HIGH);
  delay(10);
  sensorValue = digitalRead(WATER_SENSOR_PIN);

  Serial.print("Water Sensor Value: ");
  Serial.println(sensorValue);

  return sensorValue;
}

// 读取光敏传感器值
int readLightSensor() {
  // 从 LIGHT_SENSOR_PIN 读取光敏电阻的模拟值
  int lightIntensity = analogRead(LIGHT_SENSOR_PIN);
  
  // 对光敏电阻的读数进行反向处理
  int reversedIntensity = 1023 - lightIntensity;

  // 返回反向处理后的值
  return reversedIntensity;
}

// 连接到WiFi
void connectToWiFi() {
  int numNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numNetworks; i++) {
    for (int j = 0; j < sizeof(ssidList) / sizeof(ssidList[0]); j++) {
      if (WiFi.SSID(i) == ssidList[j]) {
        Serial.print("连接到 ");
        Serial.println(ssidList[j]);

        WiFi.begin(ssidList[j], passList[j]);

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
          delay(500);
          Serial.print(".");
          attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("");
          Serial.println("WiFi已连接");
          return;
        } else {
          Serial.println("");
          Serial.println("连接到WiFi失败");
        }
      }
    }
  }

  Serial.println("");
  Serial.println("未找到匹配的WiFi。请检查您的凭据。");
}
