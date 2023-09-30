#include <Arduino.h>
#include <M5Core2.h>
// カラーコードの定義
#define BLACK 0x0000
#define GREEN 0x07E0
#define BLUE  0x001F

//core2 for M5Stack
// バーグラフの設定
int barWidth = 20;  // バーグラフの幅
int barHeight = 100; // バーグラフの最大高さ
int barSpacing = 30; // バーグラフ間のスペース
int startX = 50;  // 最初のバーグラフのX座標
int startY = 160; // バーグラフのベースラインのY座標

void drawBar(float value, int barIndex, uint16_t color) {
    int barX = startX + (barWidth + barSpacing) * barIndex;
    int barH = (int)(barHeight * value/90 + 50); // 90 is normalized to barHeight
    int barY = startY - barH;
    // 以前のバーグラフを消去
    M5.Lcd.fillRect(barX, startY - barHeight, barWidth, barHeight, BLACK);
    
    // 新しいバーグラフを描画
    M5.Lcd.fillRect(barX, barY, barWidth, barH, color);
}

void displayBarGraphs(float x, float y) {
    // ロールの値をバーグラフで表示
    drawBar(x, 0, GREEN);  // 0はロールのバーグラフのインデックス

    // ピッチの値をバーグラフで表示
    drawBar(y, 1, BLUE);  // 1はピッチのバーグラフのインデックス
}

float accX = 0.0F; // Define variables for storing inertial sensor data
float accY = 0.0F;
float accZ = 0.0F;
 
float gyroX = 0.0F;
float gyroY = 0.0F;
float gyroZ = 0.0F;
 
float pitch = 0.0F;
float roll = 0.0F;
float yaw = 0.0F;
 
float temp = 0.0F;

class KalmanFilter2D {
public:
  float Q[2][2] = {{0.01, 0}, {0, 0.01}};  // Process noise
  float R[2][2] = {{0.1, 0}, {0, 0.1}};    // Measurement noise
  float P[2][2] = {{0.1, 0}, {0, 0.1}};        // Error covariance
  float x[2] = {0, 0};  // State

  void update(float measurement[2]) {
    // Prediction Step (simplified for stationary process)
    float x_pred[2] = {x[0], x[1]};
    float P_pred[2][2] = {{P[0][0] + Q[0][0], P[0][1] + Q[0][1]}, {P[1][0] + Q[1][0], P[1][1] + Q[1][1]}};

    // Kalman Gain Calculation
    float temp = P_pred[0][0] * P_pred[1][1] - P_pred[0][1] * P_pred[1][0];
    float inv_denom = 1.0 / (temp + R[0][0] * R[1][1] - R[0][1] * R[1][0]);
    float K[2][2];
    K[0][0] = (P_pred[0][0] * R[1][1] - P_pred[0][1] * R[1][0]) * inv_denom;
    K[0][1] = (P_pred[0][1] * R[0][0] - P_pred[0][0] * R[0][1]) * inv_denom;
    K[1][0] = (P_pred[1][0] * R[1][1] - P_pred[1][1] * R[1][0]) * inv_denom;
    K[1][1] = (P_pred[1][1] * R[0][0] - P_pred[1][0] * R[0][1]) * inv_denom;

    // Update Step
    float innovation[2] = {measurement[0] - x_pred[0], measurement[1] - x_pred[1]};
    x[0] = x_pred[0] + K[0][0] * innovation[0] + K[0][1] * innovation[1];
    x[1] = x_pred[1] + K[1][0] * innovation[0] + K[1][1] * innovation[1];

    // Update Error Covariance
    P[0][0] = P_pred[0][0] - K[0][0] * P_pred[0][0] - K[0][1] * P_pred[1][0];
    P[0][1] = P_pred[0][1] - K[0][0] * P_pred[0][1] - K[0][1] * P_pred[1][1];
    P[1][0] = P_pred[1][0] - K[1][0] * P_pred[0][0] - K[1][1] * P_pred[1][0];
    P[1][1] = P_pred[1][1] - K[1][0] * P_pred[0][1] - K[1][1] * P_pred[1][1];
  }
};

KalmanFilter2D kalman;

void setup() {
  M5.begin();
  // Sensor initialization code

  M5.IMU.Init();                     // Init IMU sensor.
  M5.Lcd.fillScreen(BLACK);          // Set the screen background color to black.
  M5.Lcd.setTextColor(GREEN, BLACK); // Sets the foreground color and background color of the displayed text.
  M5.Lcd.setTextSize(2);             // Set the font size.

}

void loop() {
  float x=0,y=0,z=0;
  float measurement[2] = {0, 0};
 // float Roll, Pitch;
  M5.update();
  displayBarGraphs(0, 0);
  // Stores the triaxial gyroscope data of the inertial sensor to the relevant variable
  M5.IMU.getGyroData(&gyroX, &gyroY, &gyroZ);
  M5.IMU.getAccelData(&accX, &accY, &accZ); // Stores the triaxial accelerometer.
  M5.IMU.getAhrsData(&pitch, &roll, &yaw);  // Stores the inertial sensor attitude.
  M5.IMU.getTempData(&temp);                // Stores the inertial sensor temperature to

  measurement[0] = gyroX;
  measurement[1] = gyroY;

  M5.Lcd.setCursor(0, 20);               // Move the cursor position to (x,y).
  M5.Lcd.printf("Gyro,  KalmanGyro"); // Screen printingformatted string.
  M5.Lcd.setCursor(0, 42);
  M5.Lcd.printf("%6.2f %6.2f o/s", measurement[0]*20, kalman.x[0]*20);
 
  // Accelerometer output is related
 /* M5.Lcd.setCursor(0, 70);
  M5.Lcd.printf("accX,   accY,  accZ");
  M5.Lcd.setCursor(0, 92);
  M5.Lcd.printf("%5.2f  %5.2f  %5.2f G", accX, accY, accZ);
 */
  // Pose output is related
 // M5.Lcd.setCursor(0, 120);
 // M5.Lcd.printf("Kalman Filtered X, Y");
  //M5.Lcd.printf("pitch,  roll,  yaw");

  //M5.Lcd.printf("%5.2f  %5.2f  %5.2f deg", pitch, roll, yaw);
 
  // Inertial sensor temperature output related

  //M5.Lcd.printf("Temperature : %.2f C", temp);

  kalman.update(measurement);
  //M5.Lcd.setCursor(0, 142);
  //M5.Lcd.printf("Filtered X: %.2f\n", kalman.x[0]);
  //M5.Lcd.setCursor(0, 175);
  //M5.Lcd.printf("Filtered Y: %.2f\n", kalman.x[1]);


  // バーグラフとして表示
  displayBarGraphs(measurement[0], kalman.x[0]);
  delay(50);

}
