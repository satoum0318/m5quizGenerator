#include <M5Core2.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <FS.h>

#define MAX_HISTORY 2
#define ResponseLength 250
// グローバル変数の定義
String ssid = "";
String password = "";
String CHATGPT_API_KEY = "";
const String CHATGPT_ENDPOINT = "https://api.openai.com/v1/chat/completions";
String userInput = "Let's play True or False game with me!";
String conversationHistory[MAX_HISTORY];
int historyIndex = 0;

String readFile(const char *path) {
    File file = SD.open(path);
    if (!file) {
        return "Error opening file";
    }
    String content = "";
    while (file.available()) {
        content += (char)file.read();
    }
    file.close();
    return content;
}

String callChatGPT(String userInput) {
    HTTPClient http;
    http.begin(CHATGPT_ENDPOINT);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + CHATGPT_API_KEY);

    String messages = "";
    for (int i = 0; i < MAX_HISTORY; i++) {
        if (conversationHistory[i].length() > 0) {
            if (messages != "") {
                messages += ",";
            }
            messages += "{\"role\": \"user\", \"content\": \"" + conversationHistory[i] + "\"}";
        }
    }

    String requestBody = "{\"model\": \"gpt-3.5-turbo\", \"messages\": [" + messages + "], \"temperature\": 0.2}";

    int httpCode = http.POST(requestBody);
    String responseText = "エラー: " + String(httpCode);

    // 以下の部分でAPIの応答を受け取っているので、ここで文字数のチェックとカットを行います。
    if (httpCode == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(12128);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            responseText = doc["choices"][0]["message"]["content"].as<String>();
            // ここで応答の長さをチェック
            if(responseText.length() > ResponseLength) {
                // 150文字でカット
                responseText = responseText.substring(0, ResponseLength);
                // 必要であれば「...」を追加して省略されたことを示す
                responseText += "...";
            }
        } else {
            responseText = "JSON parse failed: " + String(error.c_str());
        }
    }
    http.end();
    // ChatGPTのレスポンスを履歴に追加
    conversationHistory[historyIndex % MAX_HISTORY] = responseText;
    historyIndex++;
    return responseText;
}
void setupWiFi() {
  M5.Lcd.println("Wi-Fi connecting...");
  WiFi.begin(ssid.c_str(), password.c_str());
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) { // 10秒以内に接続を試みる
    delay(500);
    M5.Lcd.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    M5.Lcd.println("\nWi-Fi connected!");
  } else {
    M5.Lcd.println("\nWi-Fi connection failed!");
  }
}
void setup() {
  M5.begin();
  M5.Lcd.setBrightness(200);
  M5.Lcd.setTextSize(2);

  if (!SD.begin()) {
    M5.Lcd.println("SD Card initialization failed!");
    return;
  }

    String wifiContent = readFile("/wifi.txt");
    int newlinePos = wifiContent.indexOf('\n');
    // 最初の行はSSID、次の行はパスワード
    ssid = wifiContent.substring(0, newlinePos);
    ssid.trim();
    password = wifiContent.substring(newlinePos + 1);
    password.trim();
    CHATGPT_API_KEY = readFile("/api.txt");
    CHATGPT_API_KEY.trim();

  setupWiFi();

  // M5.Lcd.println("Let's talk with GPT-3.");
  M5.Lcd.println("True or False game.");
  M5.Lcd.println("Coded by monohito");
  M5.Lcd.println("A: True");
  M5.Lcd.println("B: False");
  M5.Lcd.println("C: New game.");
  delay(1000);
}

void loop() {
  M5.update();
    if (M5.BtnA.wasReleased()) {
        M5.Lcd.clear();
        delay(500);
        M5.Lcd.setCursor(0, 20);
        userInput = "true. ";
            // ユーザーの入力を履歴に追加
        conversationHistory[historyIndex % MAX_HISTORY] = userInput;
        historyIndex++;
        M5.Lcd.println(callChatGPT(userInput));
        delay(500);
        M5.Lcd.println("continue? C: New game.");
    }
    else if (M5.BtnB.wasReleased()) {
        M5.Lcd.clear();
        delay(500);
        M5.Lcd.setCursor(0, 20);
        userInput = "false. ";
            // ユーザーの入力を履歴に追加
        conversationHistory[historyIndex % MAX_HISTORY] = userInput;
        historyIndex++;
        M5.Lcd.println(callChatGPT(userInput));
        delay(500);
        M5.Lcd.println("continue? C: New game.");
    }
    else if (M5.BtnC.wasReleased()) {
        M5.Lcd.clear();
        delay(500);
        M5.Lcd.setCursor(0, 20);

        // historyをリセット
        for (int i = 0; i < MAX_HISTORY; i++) {
            conversationHistory[i] = "";
        }
        historyIndex = 1;  // indexもリセット

        userInput = "Give me a unique true or false question you haven't given before.";

            // ユーザーの入力を履歴に追加
        conversationHistory[historyIndex % MAX_HISTORY] = userInput;
        historyIndex++;
        M5.Lcd.println(callChatGPT(userInput));
        delay(500);
        M5.Lcd.println("A:True, B:False");
    }

  delay(10);
}
