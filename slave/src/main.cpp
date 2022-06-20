// Bluetooth接続スレーブ側プログラム
#include <M5StickC.h>
#include "BluetoothSerial.h"
#include <arduinoFFT.h>
#include <string>
#include <stdio.h>

#define SCALE_C 0
#define SCALE_D 1
#define SCALE_E 2
#define SCALE_F 3
#define SCALE_G 4
#define SCALE_A 5
#define SCALE_B 6
#define SCALE_C_ 7


BluetoothSerial SerialBT;
// スピーカー出力ピンの番号
const uint8_t SPEAKER_PIN = GPIO_NUM_26;

// LOWでLED点灯、HIGHでLED消灯
const uint8_t LED_ON = LOW;
const uint8_t LED_OFF = HIGH;

// PWM出力のチャンネル
const uint8_t PWM_CHANNEL = 0;
// PWM出力の分解能
const uint8_t PWM_RESOLUTION = 8;
// PWM出力の周波数
const uint32_t PWM_FREQUENCY = getApbFrequency() / (1U << PWM_RESOLUTION);
// 音声データのサンプリング周波数(Hz)
const uint32_t SOUND_SAMPLING_RATE = 8000U;

// 変数宣言
String name = "BT_Slave"; // 接続名を設定
int btn_pw = 0;           // 電源ボタン状態取得用
String data = "";         // 受信データ格納用

int lastMs = 0;
int tick = 0;
String scale_name = "";        // ドレミのどれか
static int SCALE = 0;

#define SAMPLE_PERIOD 5    // サンプリング間隔(ミリ秒)

const uint16_t FFTsamples = 32; //This value MUST ALWAYS be a power of 2
double vReal[FFTsamples];
double vImag[FFTsamples];
const double samplingFrequency = 1000.0 / (double)SAMPLE_PERIOD; // 200Hz
arduinoFFT FFT = arduinoFFT(vReal, vImag, FFTsamples, samplingFrequency);  // FFTオブジェクトを作る

int Y0 = 10;
int _height = 80 - Y0;
int _width = 160;
float dmax = 1000.0;
static float dmin_;


void drawChart(int nsamples) {
    int band_width = floor(_width / nsamples);
    int band_pad = band_width - 1;
    float dmin = vReal[0];
    for (int band = 0; band < nsamples; band++) {
        int hpos = band * band_width;
        float d = vReal[band];
        if (d < dmin) {
            dmin = d;
        }
        if (d > dmax) {
            d = dmax;
        }
        int h = (int)((d / dmax) * (_height));
        //M5.Lcd.fillRect(hpos, _height - h, band_pad, h, WHITE);
        //if ((band % 8) == 0) {
        //    M5.Lcd.setCursor(hpos, _height);
        //    M5.Lcd.printf("%dHz", (int)((band * 1.0 * samplingFrequency) / FFTsamples));
        //}
    }
    dmin_ = dmin;
}

void DCRemoval(double *vData, uint16_t samples) {
    double mean = 0;
    for (uint16_t i = 0; i < samples; i++) {
        mean += vData[i];
    }
    mean /= samples;
    for (uint16_t i = 0; i < samples; i++) {
        vData[i] -= mean;
    }
}

void drawGraph() {
    for (int i = 0; i < FFTsamples; i++) {  // 振動をサンプリングする
        float ax, ay, az;  // 加速度データを読み出す変数
        long t = micros();
        M5.MPU6886.getAccelData(&ax,&ay,&az);  // MPU6886から加速度を取得
        vReal[i] = az * 1000;  // mGに変換
        vImag[i] = 0;
        delayMicroseconds(SAMPLE_PERIOD * 1000 - (micros() - t));
    }

    DCRemoval(vReal, FFTsamples);  // 直流分を除去
    FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // 窓関数
    FFT.Compute(FFT_FORWARD); // FFT処理(複素数で計算)
    FFT.ComplexToMagnitude(); // 複素数を実数に変換

    double x = FFT.MajorPeak();
    

    //M5.Lcd.fillScreen(BLACK);  // 画面をクリア
    drawChart(FFTsamples / 2);
    //M5.Lcd.setCursor(40, 0);
    //M5.Lcd.printf("min: %.0f", dmin_);
    
}


bool isHit(void){
    
    return dmin_ > 120;
}

void oneUpSound() {
    ledcWriteTone(PWM_CHANNEL, 1318.510); //ミ
    delay(125);
    ledcWriteTone(PWM_CHANNEL, 1567.982); //ソ
    delay(125);
    ledcWriteTone(PWM_CHANNEL, 2637.020); //ミ
    delay(125);
    ledcWriteTone(PWM_CHANNEL, 2093.005); //ド
    delay(125);
    ledcWriteTone(PWM_CHANNEL, 2349.318); //レ
    delay(125);
    ledcWriteTone(PWM_CHANNEL, 3135.963); //ソ
    delay(125);
    ledcWriteTone(PWM_CHANNEL, 0); 
}

/*音声分岐処理*/
void whichSound(String data){
  int num = atoi(data.c_str());
  if (num > 69.6){
    scale_name = "C_";
    SCALE = SCALE_C_;
  } else if ((46.4 < num) && (num <= 69.6)){
    scale_name = "B";
    SCALE = SCALE_B;
  } else if ((23.2 < num) && (num <= 46.4)){
    scale_name = "A";
    SCALE = SCALE_A;
  } else if ((0 < num) && (num <= 23.2)){
    scale_name = "G";
    SCALE = SCALE_G;
  } else if ((-23.2 < num) && (num <= 0)){
    scale_name = "F";
    SCALE = SCALE_F;
  } else if ((-46.4 < num) && (num <= -23.2)){
    scale_name = "E";
    SCALE = SCALE_E;
  } else if ((-69.6 < num) && (num <= -46.4)){
    scale_name = "D";
    SCALE = SCALE_D;
  } else {
    scale_name = "C";
    SCALE = SCALE_C;
  }
}

void beatSound(int sound){
  switch (sound){
  case SCALE_C:
    ledcWriteTone(PWM_CHANNEL, 523.251); //ド
    delay(125);
    ledcWriteTone(PWM_CHANNEL, 0); 
    break;
  
  case SCALE_D:
    ledcWriteTone(PWM_CHANNEL, 	587.330); //レ
    delay(125);
    ledcWriteTone(PWM_CHANNEL, 0); 
    break;
  
  case SCALE_E:
    ledcWriteTone(PWM_CHANNEL, 659.255); //ミ
    delay(125);
    ledcWriteTone(PWM_CHANNEL, 0); 
    break;
  
  case SCALE_F:
    ledcWriteTone(PWM_CHANNEL, 	698.456); //ファ
    delay(125);
    ledcWriteTone(PWM_CHANNEL, 0);
    break;
  
  case SCALE_G:
    ledcWriteTone(PWM_CHANNEL, 783.991); //ソ
    delay(125);
    ledcWriteTone(PWM_CHANNEL, 0); 
    break;
  
  case SCALE_A:
    ledcWriteTone(PWM_CHANNEL, 	880.000); //ラ
    delay(125);
    ledcWriteTone(PWM_CHANNEL, 0); 
    break;
  
  case SCALE_B:
    ledcWriteTone(PWM_CHANNEL, 	987.767); //シ
    delay(125);
    ledcWriteTone(PWM_CHANNEL, 0); 
    break;
  
  case SCALE_C_:
    ledcWriteTone(PWM_CHANNEL, 1046.502); //ド
    delay(125);
    ledcWriteTone(PWM_CHANNEL, 0); 
    break;
  
  default:
    break;
  }
}

// 初期設定 -----------------------------------------------
void setup() {
  M5.begin();             // 本体初期化
  Serial.begin(9600);     // シリアル通信初期化
  M5.MPU6886.Init();
  // 出力設定
  pinMode(10, OUTPUT);    //本体LED赤
  digitalWrite(10, HIGH); //本体LED初期値OFF（HIGH）
  // LCD表示設定
  M5.Lcd.setTextFont(2);
  M5.Lcd.println(name); // 接続名表示

  // Bluetooth接続開始
  SerialBT.begin(name); // 接続名を指定して初期化。第2引数にtrueを書くとマスター、省略かfalseでスレーブ
  // MACアドレスの取得と表示
  uint8_t macBT[6];
  esp_read_mac(macBT, ESP_MAC_BT);  // MACアドレス取得
  M5.Lcd.printf("%02X:%02X:%02X:%02X:%02X:%02X\r\n", macBT[0], macBT[1], macBT[2], macBT[3], macBT[4], macBT[5]);
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\r\n", macBT[0], macBT[1], macBT[2], macBT[3], macBT[4], macBT[5]);
  // 電源ON時のシリアルデータが無くなるまで待つ
  while (Serial.available()) {data = Serial.read();}
  M5.Lcd.setTextFont(1);  // テキストサイズ変更

  // スピーカーの設定
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(SPEAKER_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0);

  // LEDの設定
  pinMode(M5_LED, OUTPUT);
  digitalWrite(M5_LED, LED_ON);
  
  //初期設定終了 LED OFF
  digitalWrite(M5_LED, LED_OFF);  
}
// メイン -------------------------------------------------
void loop() {
  M5.update();  // 本体のボタン状態更新
  drawGraph();
 
  // データ受信時処理
  // スレーブからの受信データをLCDに表示
  if (SerialBT.available()) {               // Bluetoothデータ受信で
    data = SerialBT.readStringUntil('\r');  // 区切り文字「CR」の手前までデータ取得
    M5.Lcd.setCursor(0, 1);
    M5.Lcd.fillScreen(BLACK);
    //M5.Lcd.print("BT: ");                   // 受信データ液晶表示
    //M5.Lcd.println(data);                   // 液晶表示は改行あり
    //Serial.print(data);                     // シリアル出力は改行なし
    whichSound(data);
    M5.Lcd.setTextSize(8);  // テキストサイズ変更
    M5.Lcd.println(scale_name); 
  }
  // シリアル入力データ（「CR」付きで入力）をスレーブ側へ送信
  if (Serial.available()) {               // シリアルデータ受信で
    data = Serial.readStringUntil('\r');  // 「CR」の手前までデータ取得
    M5.Lcd.setCursor(0, 1);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.print("SR: ");                 // シリアルデータ液晶表示
    M5.Lcd.println(data);                 // 液晶表示は改行あり
    Serial.print(data);                   // シリアル出力は改行なし
    // Bluetoothデータ送信
    SerialBT.print(data + "\r"); // 区切り文字「CR」をつけてマスター側へ送信
  }

      


  // ボタンが押された場合の処理
  if (isHit())
  {   
    if(SerialBT.available()){
       // LED点灯
      digitalWrite(M5_LED, LED_ON);

      beatSound(SCALE);

      // LED消灯
      digitalWrite(M5_LED, LED_OFF);
    }
     
  }

  // 再起動（リスタート）処理
  if (btn_pw == 2) {          // 電源ボタン短押し（1秒以下）なら
    ESP.restart();
  }
  // 電源ボタン状態取得（1秒以下のONで「2」1秒以上で「1」すぐに「0」に戻る）
  btn_pw = M5.Axp.GetBtnPress();
  delay(5);  // 5ms遅延時間
}