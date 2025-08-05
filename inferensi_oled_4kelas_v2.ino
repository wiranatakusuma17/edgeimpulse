#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <UAS_SINYAL_SISEM_FIX_inferencing.h> // Library Hasil Edge Impulse
#include "edge-impulse-sdk/dsp/image/image.hpp"

#include <U8g2lib.h>  // OLED U8g2

#define FREQUENCY_HZ        100
#define INTERVAL_MS         (1000 / FREQUENCY_HZ)

#define BLUE 2

Adafruit_MPU6050 mpu;
float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
size_t feature_ix = 0;
static unsigned long last_interval_ms = 0;

// OLED SETUP
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE); 
String current_label = "";

void setup() {
  Serial.begin(115200);

  pinMode(BLUE, OUTPUT);

  // OLED Init
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 15, "Edge Impulse Ready");
  u8g2.sendBuffer();

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) delay(10);
  }

  Serial.println("MPU6050 Found!");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.print("Features: ");
  Serial.println(EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
  Serial.print("Label count: ");
  Serial.println(EI_CLASSIFIER_LABEL_COUNT);
  delay(100);
}

void loop() {
  sensors_event_t a, g, temp;

  if (millis() > last_interval_ms + INTERVAL_MS) {
    last_interval_ms = millis();

    mpu.getEvent(&a, &g, &temp);

    // Transformasi orientasi sensor
    float ax = a.acceleration.y;
    float ay = -a.acceleration.x;
    float az = a.acceleration.z;

    features[feature_ix++] = ax;
    features[feature_ix++] = ay;
    features[feature_ix++] = az;

    if (feature_ix == EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
      Serial.println("\nRunning inference...");

      signal_t signal;
      ei_impulse_result_t result;
      int err = numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
      if (err != 0) {
        ei_printf("Failed to create signal from buffer (%d)\n", err);
        return;
      }

      EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
      if (res != EI_IMPULSE_OK) return;

      ei_printf("Predictions (DSP: %d ms., Classification: %d ms.):\n",
                result.timing.dsp, result.timing.classification);

      digitalWrite(BLUE, LOW);

      for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("  %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);

        if (result.classification[ix].value > 0.6) {
          current_label = result.classification[ix].label; // var OLED

          if (strcmp(result.classification[ix].label, "diam") == 0) {
          
            Serial.println("Detected: DIAM");

          } else if (strcmp(result.classification[ix].label, "mengetik") == 0) {
          
            Serial.println("Detected: MENGEIK");

          } else if (strcmp(result.classification[ix].label, "berjalan") == 0) {
      
            Serial.println("Detected: BERJALAN");

          } else if (strcmp(result.classification[ix].label, "berlari") == 0) {
            digitalWrite(BLUE, HIGH);
            Serial.println("Detected: LARI");

          } else {
            Serial.println("Detected: gesture tidak dikenal");
          }
        }
      }

      // === OLED Update ===
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_fub17_tf); 
      u8g2.drawStr(0, 15, current_label.c_str());
      u8g2.sendBuffer();

      feature_ix = 0;
    }
  }
}

void ei_printf(const char *format, ...) {
  static char print_buf[1024];
  va_list args;
  va_start(args, format);
  int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
  va_end(args);
  if (r > 0) {
    Serial.write(print_buf);
  }
}
