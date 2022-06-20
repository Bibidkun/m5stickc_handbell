// Bluetooth接続マスター側プログラム
#include <M5StickC.h>
#include "BluetoothSerial.h"
#include <Kalman.h>
BluetoothSerial SerialBT;

// 変数宣言
String slave_name = "BT_Slave";   // スレーブ側の接続名
String master_name = "BT_master"; // マスターの接続名
uint8_t address[6]  = {0xD8, 0xA0, 0x1D, 0x55, 0x34, 0xDE};  // MACアドレスで接続の場合、スレーブ側のMACアドレスを設定
bool connected = 0;       // 接続状態格納用
int connect_count = 3;    // 再接続実行回数
String data = "";         // 受信データ格納用
int btn_pw = 0;           // 電源ボタン状態取得用

//x, y, zの順
float acc[3];
float accOffset[3];
float gyro[3];
float gyroOffset[3];
float kalAngleX;
float kalAngleY;
Kalman kalmanX;
Kalman kalmanY;
long lastMs = 0;


// 再起動（リスタート）処理
void restart() {
  // 電源ボタン状態取得（1秒以下のONで「2」1秒以上で「1」すぐに「0」に戻る）
  btn_pw = M5.Axp.GetBtnPress();
  if (btn_pw == 2) {  // 電源ボタン短押し（1秒以下）なら
    ESP.restart();    // 再起動
  }
}

void readGyro(){
  M5.MPU6886.getGyroData(&gyro[0], &gyro[1], &gyro[2]);
  M5.MPU6886.getAccelData(&acc[0], &acc[1], &acc[2]);
}
void calibration(){
  //補正値を求める
  float gyroSum[3];
  float accSum[3];
  for(int i = 0; i < 500; i++){
    readGyro();
    gyroSum[0] += gyro[0];
    gyroSum[1] += gyro[1];
    gyroSum[2] += gyro[2];
    accSum[0] += acc[0];
    accSum[1] += acc[1];
    accSum[2] += acc[2];
    delay(2);
  }
  gyroOffset[0] = gyroSum[0]/500;
  gyroOffset[1] = gyroSum[1]/500;
  gyroOffset[2] = gyroSum[2]/500;
  accOffset[0] = accSum[0]/500;
  accOffset[1] = accSum[1]/500;
  accOffset[2] = accSum[2]/500 - 1.0;//重力加速度1G
}

void applyCalibration(){
  gyro[0] -= gyroOffset[0];
  gyro[1] -= gyroOffset[1];
  gyro[2] -= gyroOffset[2];
  acc[0] -= accOffset[0];
  acc[1] -= accOffset[1];
  acc[2] -= accOffset[2];
}
float getRoll(){
  return atan2(acc[1], acc[2]) * RAD_TO_DEG;
}
float getPitch(){
  return atan(-acc[0] / sqrt(acc[1]*acc[1] + acc[2]*acc[2])) * RAD_TO_DEG;
}


// 初期設定 -----------------------------------------------
void setup() {
  M5.begin();             // 本体初期化
  Serial.begin(9600);     // シリアル通信初期化
  // 出力設定
  pinMode(10, OUTPUT);    //本体LED赤
  digitalWrite(10, HIGH); //本体LED初期値OFF（HIGH）
  // LCD表示設定
  M5.Lcd.setTextFont(2);

  M5.MPU6886.Init();
  calibration();
  readGyro();
  kalmanX.setAngle(getRoll());
  kalmanY.setAngle(getPitch());

  // Bluetooth接続開始
  SerialBT.begin(master_name, true);  // マスターとして初期化。trueを書くとマスター、省略かfalseを指定でスレーブ
  M5.Lcd.print("BT Try!\n.");
  Serial.print("BT Try!\n.");
  // connect(address)は高速（最大10秒）、connect(slave_name)は低速（最大30秒）
  // slave_nameでの接続はMACアドレスを調べなくても良いが接続は遅い
  while (connected == 0) {    // connectedが0(未接続)なら接続実行を繰り返す
    if (connect_count != 0) { // 再接続実行回数カウントが0でなければ
      connected = SerialBT.connect(slave_name); // 接続実行（接続名で接続する場合）
      // connected = SerialBT.connect(address);   // 接続実行（MACアドレスで接続する場合）
      M5.Lcd.print(".");
      connect_count--;        // 再接続実行回数カウント -1
    } else {                  // 再接続実行回数カウントが0なら接続失敗
      M5.Lcd.setTextColor(RED, BLACK);
      M5.Lcd.print("\nFailed!");
      Serial.print("\nFailed!");
      while (1) {restart();}  // 無限ループ(再起動待機)
    }
  }
  // 接続確認
  M5.Lcd.setTextColor(WHITE, BLACK);
  if (connected) {                    // 接続成功なら
    M5.Lcd.setTextColor(CYAN, BLACK);
    M5.Lcd.println("\nConnected!");   // 「Connected!」表示
    Serial.println("\nConnected!");
  } else {                            // 接続失敗なら
    M5.Lcd.setTextColor(RED, BLACK);
    M5.Lcd.println("\nFailed!!");     // 「Failed!!」表示
    Serial.println("\nFailed!!");
    while (1) {restart();}            // 無限ループ(再起動待機)
  }
  delay(1000);                        // 接続結果確認画面表示時間
  // 電源ON時のシリアルデータが無くなるまで待つ
  while (Serial.available()) {data = Serial.read();}
  // LCD表示リセット
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.println(master_name);        // 接続名表示

  lastMs = micros();
}


// メイン -------------------------------------------------
void loop() {
  M5.update();  // 本体ボタン状態更新
  readGyro();
  applyCalibration();
  float dt = (micros() - lastMs) / 1000000.0;
  lastMs = micros();
  float roll = getRoll();
  float pitch = getPitch();
  
  kalAngleX = kalmanX.getAngle(roll, gyro[0], dt);
  kalAngleY = kalmanY.getAngle(pitch, gyro[1], dt);

  if (connected == 1) {
    SerialBT.printf("%7.2f deg\r", kalAngleX);     // 傾斜角を送信
  }

  // ボタン操作処理（スレーブ側へデータ送信）※送信のみでよければコメント部を有効にして省エネ
  if (M5.BtnA.wasPressed()) {     // ボタンAが押されていれば
    M5.MPU6886.Init();
    calibration();
    readGyro();
    kalmanX.setAngle(getRoll());
    kalmanY.setAngle(getPitch());
  }
  

  /* ディスプレイ処理 */  
  // TODO

  restart();  // 再起動（リスタート）処理
  delay(5);  // 5ms遅延時間
}