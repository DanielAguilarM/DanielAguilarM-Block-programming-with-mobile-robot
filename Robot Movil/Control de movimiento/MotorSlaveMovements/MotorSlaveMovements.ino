#include <esp_now.h>
#include <WiFi.h>

char message[32];
int SetPoint = 0;
String command;
float value1=0.0;
float value2=0.0;
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    memcpy(&message, incomingData, sizeof(message));
    command = String(message);
}

class Motor {
public:
    Motor(const int pin_1, const int pin_2, const int pwm, const int pwm_channel) : pin_1(pin_1), pin_2(pin_2), pwm(pwm), pwm_channel(pwm_channel) {}
    const int pin_1;
    const int pin_2;
    const int pwm;
    const int pwm_channel;
    float output;
    void initMotor() {
        pinMode(pin_1, OUTPUT);
        pinMode(pin_2, OUTPUT);
        ledcSetup(pwm_channel, 5000, 8);
        ledcAttachPin(pwm, pwm_channel);
    }
    void runFront() {
        digitalWrite(pin_1, HIGH);
        digitalWrite(pin_2, LOW);
    }
    void runBack() {
        digitalWrite(pin_1, LOW);
        digitalWrite(pin_2, HIGH);
    }
    void Stop() {
        digitalWrite(pin_1, LOW);
        digitalWrite(pin_2, LOW);
    }
    void updatePWM(int value) {
        ledcWrite(pwm_channel, value);
    }
};

class Encoder {
public:
    int pin_a;
    int pin_b;
    float input;
    float input_b;
    volatile int counter_A = 0;
    volatile int counter_B = 0;
    Encoder(int a, int b) : pin_a(a), pin_b(b) {}

    void initEncoder() {
        pinMode(pin_a, INPUT);
        pinMode(pin_b, INPUT);
    }
};

unsigned long previous_Millis = 0;
long interval = 10;

class PIDController {
private:
    float SetPoint;
    float Kp, Ki, Kd, Tm;
    float cv1, error1, error2;

public:
    PIDController(float _Kp, float _Ki, float _Kd, float _Tm) {
        Kp = _Kp;
        Ki = _Ki;
        Kd = _Kd;
        Tm = _Tm;
        cv1 = 0.0;
        error1 = 0.0;
        error2 = 0.0;
    }

    float calculate(float pv) {
        float error = SetPoint - pv;
        float term_1 = (Kp + Kd / Tm) * error;
        float term_2 = (-Kp + Ki * Tm - 2 * Kd / Tm) * error1;
        float term_3 = (Kd / Tm) * error2;

        float cv = cv1 + term_1 + term_2 + term_3;

        cv1 = cv;
        error2 = error1;
        error1 = error;

        return cv;
    }

    void setSetPoint(float sp) {
        SetPoint = sp;
    }
};

Motor MotorTopLeft(22, 23, 21, 0);
Motor MotorBackLeft(25, 33, 32, 1);
Motor MotorTopRight(2, 15, 0, 2);
Motor MotorBackRight(13, 12, 14, 3);

Encoder EncoderTopLeft(36, 39);
Encoder EncoderBackLeft(19, 18);
Encoder EncoderTopRight(26, 27);
Encoder EncoderBackRight(16, 17);

float Tm = 0.1;
float Kp = 0.52998;
float Ki = 3.583;
float Kd = 0.0058866;

PIDController motorTOPLEFT_controller(Kp, Ki, Kd, Tm);
PIDController motorBACKLEFT_controller(Kp, Ki, Kd, Tm);
PIDController motorTOPRIGHT_controller(Kp, Ki, Kd, Tm);
PIDController motorBACKRIGHT_controller(Kp, Ki, Kd, Tm);

float avgInputTopLeft = 0;
float avgInputBackLeft = 0;
float avgInputTopRight = 0;
float avgInputBackRight = 0;
const int filterSize = 5;
float filterBufferTopLeft[filterSize] = {0};
float filterBufferBackLeft[filterSize] = {0};
float filterBufferTopRight[filterSize] = {0};
float filterBufferBackRight[filterSize] = {0};
int filterIndex = 0;

void setup() {
    Serial.begin(115200);
    MotorTopLeft.initMotor();
    MotorTopRight.initMotor();
    MotorBackLeft.initMotor();
    MotorBackRight.initMotor();
    EncoderTopLeft.initEncoder();
    EncoderBackLeft.initEncoder();
    EncoderTopRight.initEncoder();
    EncoderBackRight.initEncoder();
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_recv_cb(OnDataRecv);
    attachInterrupt(digitalPinToInterrupt(EncoderTopLeft.pin_a), counter_encoder_TOP_LEFT_a, RISING);
    attachInterrupt(digitalPinToInterrupt(EncoderTopLeft.pin_b), counter_encoder_TOP_LEFT_b, RISING);
    attachInterrupt(digitalPinToInterrupt(EncoderBackLeft.pin_a), counter_encoder_BACK_LEFT_a, RISING);
    attachInterrupt(digitalPinToInterrupt(EncoderBackLeft.pin_b), counter_encoder_BACK_LEFT_b, RISING);
    attachInterrupt(digitalPinToInterrupt(EncoderTopRight.pin_a), counter_encoder_TOP_RIGHT_a, RISING);
    attachInterrupt(digitalPinToInterrupt(EncoderTopRight.pin_b), counter_encoder_TOP_RIGHT_b, RISING);
    attachInterrupt(digitalPinToInterrupt(EncoderBackRight.pin_a), counter_encoder_BACK_RIGHT_a, RISING);
    attachInterrupt(digitalPinToInterrupt(EncoderBackRight.pin_b), counter_encoder_BACK_RIGHT_b, RISING);
    Stop();
    motorTOPLEFT_controller.setSetPoint(0);
    motorBACKLEFT_controller.setSetPoint(0);
    motorTOPRIGHT_controller.setSetPoint(0);
    motorBACKRIGHT_controller.setSetPoint(0);

    // Inicializa los valores de PWM para sincronizar los motores
}

void loop() {
    unsigned long current_Millis = millis();
    if ((current_Millis - previous_Millis) >= interval) {
        previous_Millis = current_Millis;
        EncoderTopLeft.input = (EncoderTopLeft.counter_A + EncoderTopLeft.counter_B) * (60.0 / 748.0);
        EncoderBackLeft.input = (EncoderBackLeft.counter_A + EncoderBackLeft.counter_B) * (60.0 / 748.0);
        EncoderTopRight.input = (EncoderTopRight.counter_A + EncoderTopRight.counter_B) * (60.0 / 748.0);
        EncoderBackRight.input = (EncoderBackRight.counter_A + EncoderBackRight.counter_B) * (60.0 / 748.0);

        // Aplica el filtro de promedio móvil
        filterBufferTopLeft[filterIndex] = EncoderTopLeft.input;
        filterBufferBackLeft[filterIndex] = EncoderBackLeft.input;
        filterBufferTopRight[filterIndex] = EncoderTopRight.input;
        filterBufferBackRight[filterIndex] = EncoderBackRight.input;

        avgInputTopLeft = 0;
        avgInputBackLeft = 0;
        avgInputTopRight = 0;
        avgInputBackRight = 0;
        for (int i = 0; i < filterSize; i++) {
            avgInputTopLeft += filterBufferTopLeft[i];
            avgInputBackLeft += filterBufferBackLeft[i];
            avgInputTopRight += filterBufferTopRight[i];
            avgInputBackRight += filterBufferBackRight[i];
        }
        avgInputTopLeft /= filterSize;
        avgInputBackLeft /= filterSize;
        avgInputTopRight /= filterSize;
        avgInputBackRight /= filterSize;

        filterIndex = (filterIndex + 1) % filterSize;

        
    }
    reset_Counters();

    MotorTopLeft.output = motorTOPLEFT_controller.calculate(avgInputTopLeft);
    MotorBackLeft.output = motorBACKLEFT_controller.calculate(avgInputBackLeft);
    MotorTopRight.output = motorTOPRIGHT_controller.calculate(avgInputTopRight);
    MotorBackRight.output = motorBACKRIGHT_controller.calculate(avgInputBackRight);

    MotorTopLeft.updatePWM(constrain(MotorTopLeft.output * (255.0 / 748.0), 0, 255)+value1);
    MotorBackLeft.updatePWM(constrain(MotorBackLeft.output * (255.0 / 748.0), 0, 255)+value1);
    MotorTopRight.updatePWM(constrain(MotorTopRight.output * (255.0 / 748.0), 0, 255)+value2);
    MotorBackRight.updatePWM(constrain(MotorBackRight.output * (255.0 / 748.0), 0, 255)+value2);

    // Compensación activa para corregir la desviación
    //syncMotors();
    if (command == "Up") {
        SetPoint = 100;
        value1=0.0;
        value2=-5.6;
        Up();
    } else if (command == "Down") {
        SetPoint = 100;
        value1=-5.4;
        value2=0;
        Down();
    } else if (command == "Left") {
        SetPoint = 100;
        value1=0.0;
        value2=0.0;
        Left();
    } else if (command == "Right") {
        SetPoint = 100;
        value1=0.0;
        value2=0.0;
        Right();
    } else if (command == "DFLeft") {
        SetPoint = 100;
        value1=0.0;
        value2=0.0;
        DiagonalFrontLeft();
    } else if (command == "DFRight") {
        SetPoint = 100;
        value1=0.0;
        value2=0.0;
        DiagonalFrontRight();
    } else if (command == "DBLeft") {
        SetPoint = 100;
        value1=0.0;
        value2=0.0;
        DiagonalBackLeft();
    } else if (command == "DBRight") {
        SetPoint = 100;
        DiagonalBackRight();
    } else if (command == "TLeft") {
        SetPoint = 120;
        value1=0.0;
        value2=0.0;
        TurnLeft();
    } else if (command == "TRight") {
        SetPoint = 120;
        value1=0.0;
        value2=0.0;
        TurnRight();
    } else if (command == "Stop") {
        SetPoint = 0;
        Stop();
    }
    motorTOPLEFT_controller.setSetPoint(SetPoint);
    motorBACKLEFT_controller.setSetPoint(SetPoint);
    motorTOPRIGHT_controller.setSetPoint(SetPoint);
    motorBACKRIGHT_controller.setSetPoint(SetPoint);

    delay(10);
}

void syncMotors() {
  const float adjustmentFactor = 5.0;
  const float avgFactor = 0.25; // Factor de promedio móvil
    // Cálculo del promedio móvil para suavizar las lecturas de los encoders
    avgInputTopLeft = avgInputTopLeft * (1 - avgFactor) + EncoderTopLeft.input * avgFactor;
    avgInputBackLeft = avgInputBackLeft * (1 - avgFactor) + EncoderBackLeft.input * avgFactor;
    avgInputTopRight = avgInputTopRight * (1 - avgFactor) + EncoderTopRight.input * avgFactor;
    avgInputBackRight = avgInputBackRight * (1 - avgFactor) + EncoderBackRight.input * avgFactor;

    float avgLeft = (avgInputTopLeft + avgInputBackLeft) / 2.0;
    float avgRight = (avgInputTopRight + avgInputBackRight) / 2.0;
    float avgFront = (avgInputTopLeft + avgInputTopRight) / 2.0;
    float avgBack = (avgInputBackLeft + avgInputBackRight) / 2.0;
    float avgOverall = (avgLeft + avgRight + avgFront + avgBack) / 4.0;

    // Ajustes finos para corregir la desviación
    float correctionFactorLeft = 0.0; // Ajuste para la izquierda
    float correctionFactorRight = 0.0; // Ajuste para la derecha

    if (avgFront > avgBack) {
        correctionFactorLeft += 1.0; // Incrementa el ajuste a la izquierda si la parte delantera va más rápido
    } else if (avgBack > avgFront) {
        correctionFactorRight += 1.0; // Incrementa el ajuste a la derecha si la parte trasera va más rápido
    }

    // Ajustes finos adicionales según la diferencia entre las velocidades de los motores
    float speedDiffLeft = avgLeft - avgRight;
    float speedDiffRight = avgRight - avgLeft;

    correctionFactorLeft += speedDiffLeft * 0.1;
    correctionFactorRight += speedDiffRight * 0.1;

    MotorTopLeft.output += ((avgOverall - avgInputTopLeft) * adjustmentFactor) + correctionFactorLeft;
    MotorBackLeft.output += ((avgOverall - avgInputBackLeft) * adjustmentFactor) + correctionFactorLeft;
    MotorTopRight.output += ((avgOverall - avgInputTopRight) * adjustmentFactor) + correctionFactorRight;
    MotorBackRight.output += ((avgOverall - avgInputBackRight) * adjustmentFactor) + correctionFactorRight;

    MotorTopLeft.output = constrain(MotorTopLeft.output * (255.0 / 142.0), 0, 255);
    MotorBackLeft.output = constrain(MotorBackLeft.output * (255.0 / 142.0), 0, 255);
    MotorTopRight.output = constrain(MotorTopRight.output * (255.0 / 142.0), 0, 255);
    MotorBackRight.output = constrain(MotorBackRight.output * (255.0 / 142.0), 0, 255);
}

void counter_encoder_TOP_LEFT_a() {
    EncoderTopLeft.counter_A++;
}

void counter_encoder_TOP_LEFT_b() {
    EncoderTopLeft.counter_B++;
}

void counter_encoder_BACK_LEFT_a() {
    EncoderBackLeft.counter_A++;
}

void counter_encoder_BACK_LEFT_b() {
    EncoderBackLeft.counter_B++;
}

void counter_encoder_TOP_RIGHT_a() {
    EncoderTopRight.counter_A++;
}

void counter_encoder_TOP_RIGHT_b() {
    EncoderTopRight.counter_B++;
}

void counter_encoder_BACK_RIGHT_a() {
    EncoderBackRight.counter_A++;
}

void counter_encoder_BACK_RIGHT_b() {
    EncoderBackRight.counter_B++;
}

void reset_Counters() {
    EncoderTopLeft.counter_A = 0;
    EncoderTopLeft.counter_B = 0;
    EncoderBackLeft.counter_A = 0;
    EncoderBackLeft.counter_B = 0;
    EncoderTopRight.counter_A = 0;
    EncoderTopRight.counter_B = 0;
    EncoderBackRight.counter_A = 0;
    EncoderBackRight.counter_B = 0;
}

void Up() {
    //Movimientos 
    MotorTopLeft.runFront();
    MotorTopRight.runFront();
    MotorBackLeft.runFront();
    MotorBackRight.runFront(); 
}

void Down() {
    //Movimientos 
    MotorTopLeft.runBack();
    MotorTopRight.runBack();
    MotorBackLeft.runBack();
    MotorBackRight.runBack();  
}

void Left() {
    //Movimientos
    MotorTopLeft.runBack();
    MotorTopRight.runFront();
    MotorBackLeft.runFront();
    MotorBackRight.runBack();  
}

void Right() {
    //Movimientos
    MotorTopLeft.runFront();
    MotorTopRight.runBack();
    MotorBackLeft.runBack();
    MotorBackRight.runFront();  
}

void DiagonalFrontLeft() {
    MotorTopLeft.Stop();
    MotorTopRight.runFront();
    MotorBackLeft.runFront();
    MotorBackRight.Stop();
}

void DiagonalFrontRight() {
    MotorTopLeft.runFront();
    MotorTopRight.Stop();
    MotorBackLeft.Stop();
    MotorBackRight.runFront();
}

void DiagonalBackLeft() {
    MotorTopLeft.runBack();
    MotorTopRight.Stop();
    MotorBackLeft.Stop();
    MotorBackRight.runBack();
}

void DiagonalBackRight() {
    MotorTopLeft.Stop();
    MotorTopRight.runBack();
    MotorBackLeft.runBack();
    MotorBackRight.Stop();
}

void TurnLeft() {
    MotorTopLeft.runBack();
    MotorTopRight.runFront();
    MotorBackLeft.runBack();
    MotorBackRight.runFront();
}

void TurnRight() {
    MotorTopLeft.runFront();
    MotorTopRight.runBack();
    MotorBackLeft.runFront();
    MotorBackRight.runBack();
}

void Stop() {
    //Movimientos
    MotorTopLeft.Stop();
    MotorTopRight.Stop();
    MotorBackLeft.Stop();
    MotorBackRight.Stop();
}
