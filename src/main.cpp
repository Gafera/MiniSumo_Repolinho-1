#define DECODE_SONY

#include <BluetoothSerial.h>
#include <analogWrite.h>
#include <Arduino.h>
#include <IRremote.hpp>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

char BT = 'f';         // char que é recebida pelo bluetooth
char estrategia = 'f'; // estratégio de início da luta (qualquer caracter)

#define sensorE 32
#define sensorD 35
#define sensorFd 39
#define sensorFe 33
#define sensorF 36

#define pwmB 17
#define b1 5
#define b2 18
#define stby 19
#define a1 3
#define a2 1
#define pwmA 23

#define led 2
#define IR 13

enum Direction
{
    esq,
    dir
};
Direction direc = esq;

#pragma region motorController
    void stop()
    {
        digitalWrite(a1, 1);
        digitalWrite(a2, 1);
        digitalWrite(b1, 1);
        digitalWrite(b2, 1);
    }
    void right(uint32_t pa, uint32_t pb)
    {
        digitalWrite(b1, 1);
        digitalWrite(b2, 0);
        digitalWrite(a1, 1);
        digitalWrite(a2, 0);
        analogWrite(pwmB, pb, 255);
        analogWrite(pwmA, pa, 255);
    }
    void left(uint32_t pa, uint32_t pb)
    {
        digitalWrite(b1, 0);
        digitalWrite(b2, 1);
        digitalWrite(a1, 0);
        digitalWrite(a2, 1);
        analogWrite(pwmB, pb, 255);
        analogWrite(pwmA, pa, 255);
    }
    void backward(uint32_t pa, uint32_t pb)
    {
        digitalWrite(b1, 0);
        digitalWrite(b2, 1);
        digitalWrite(a1, 1);
        digitalWrite(a2, 0);
        analogWrite(pwmB, pb, 255);
        analogWrite(pwmA, pa, 255);
    }
    void forward(uint32_t pa, uint32_t pb)
    {
        digitalWrite(b1, 1);
        digitalWrite(b2, 0);
        digitalWrite(a1, 0);
        digitalWrite(a2, 1);
        analogWrite(pwmB, pb, 255);
        analogWrite(pwmA, pa, 255);
    }
#pragma endregion motorController

TaskHandle_t SensorTask;

bool valueSharpE, valueSharpD, valueSharpFd, valueSharpFe;
bool running = false;
int valueSharpF;
void FunctionSensorTask(void *pvParameters)
{
    // SerialBT.print("FunctionSensorTask running on core ");
    // SerialBT.println(xPortGetCoreID());
    for (;;)
    {
        vTaskDelay(100);
        if (running)
        {
            if (IrReceiver.decode())
            {
                IrReceiver.resume();
                if (IrReceiver.decodedIRData.command == 0x2)
                {
                    digitalWrite(led, 1);
                    stop();
                    ESP.restart();
                }
                valueSharpF = analogRead(sensorF);
                valueSharpD = digitalRead(sensorD);
                valueSharpE = digitalRead(sensorE);
                valueSharpFd = digitalRead(sensorFd);
                valueSharpFe = digitalRead(sensorFe);
            }
            vTaskDelay(15);
            valueSharpF = analogRead(sensorF);
            valueSharpD = digitalRead(sensorD);
            valueSharpE = digitalRead(sensorE);
            valueSharpFd = digitalRead(sensorFd);
            valueSharpFe = digitalRead(sensorFe);
            
            if (valueSharpE || valueSharpFe)
            {
              direc = esq;
            }
            else if (valueSharpD || valueSharpFd)
            {
              direc = dir;
            }
            
        }
    }
}

#pragma region testsRegion
void testSensors()
{
    SerialBT.println("Test Sensors");
    running = true;
    for (;;)
    {
        SerialBT.printf("ESQ: %d, FrenteESQ: %d, FEN: %d, FrenteDIR: %d, DIR: %d\n", valueSharpE, valueSharpFe, valueSharpF, valueSharpFd, valueSharpD);
        delay(15);
    }
}

void testMotorsWithSensors()
{
    digitalWrite(stby, 1);
    SerialBT.println("Test Motors With Sensors");
    running = true;
    for (;;)
    {
        if (valueSharpE)
            left(90, 90);
        else if (valueSharpD)
            right(90, 90);
        else if (valueSharpFd)
            forward(90, 255);
        else if (valueSharpFe)
            forward(255, 90);
        else{
          stop();
        }
    }
}
#pragma endregion testsRegion

void setup()
{
    pinMode(pwmB, OUTPUT);
    pinMode(b1, OUTPUT);
    pinMode(b2, OUTPUT);
    pinMode(stby, OUTPUT);
    pinMode(a1, OUTPUT);
    pinMode(a2, OUTPUT);
    pinMode(pwmA, OUTPUT);
    pinMode(sensorD, INPUT);
    pinMode(sensorE, INPUT);
    pinMode(sensorF, INPUT);
    pinMode(sensorFd, INPUT);
    pinMode(sensorFe, INPUT);
    pinMode(led, OUTPUT);
    digitalWrite(stby, 0);

    IrReceiver.begin(IR, ENABLE_LED_FEEDBACK, USE_DEFAULT_FEEDBACK_LED_PIN);
    printActiveIRProtocols(&Serial);

    SerialBT.begin("Repolinho"); // Bluetooth device name
    SerialBT.println("LIGOUUUU");

    xTaskCreatePinnedToCore(FunctionSensorTask, "SensorTask", 10000, NULL, 1, &SensorTask, 0);
    delay(500);

    while (BT != '0')
    { // Inicio quando bluetooth recebe o char '0'
        if (SerialBT.available())
        {
            BT = SerialBT.read();
            SerialBT.println(BT);
        }
        if (BT == '<')
        { // definir para que lado será toda a progrmação
            direc = esq;
        }
        else if (BT == '>')
        {
            direc = dir;
        }
        else if (BT == 'C')
        { // Char 'C' checa as estrategias
            SerialBT.println("Check");
            SerialBT.print("Estrategia: ");
            SerialBT.println(estrategia);
            SerialBT.print("Direc: ");
            if (direc == dir)
                SerialBT.println("direita");
            else
                SerialBT.println("esquerda");
        }
        else if (BT == 'S')
        { // Char 'S'teste sensor
            testSensors();
        }
        else if (BT == 'M')
        { // Char 'M'teste motor com sensor
            testMotorsWithSensors();
        }
        else if (BT == '0')
        {
            break;
        }
        else
        {
            if (!(BT == 13 || BT == 10 || (BT > 48 && BT < 58)))
            { // Se não for ENTER ou numero altera a estrategia inicial
                estrategia = BT;
            }
        }
    }

    bool ready = false;
    while (true)
    {
        if (IrReceiver.decode())
        {
            IrReceiver.resume();
            if (IrReceiver.decodedIRData.command == 0x0)
            {
                ready = true;
                for (int i = 0; i < 3; i++)
                {
                    digitalWrite(led, 1);
                    delay(50);
                    digitalWrite(led, 0);
                    delay(50);
                    if (IrReceiver.decode())
                    {
                        IrReceiver.resume();
                        if (IrReceiver.decodedIRData.command == 0x1 && ready)
                        {
                            digitalWrite(led, 0);
                            digitalWrite(stby, 1);
                            break;
                        }
                    }
                }
            }

            if (IrReceiver.decodedIRData.command == 0x1 && ready)
            {
                digitalWrite(led, 0);
                digitalWrite(stby, 1);
                break;
            }
        }
    }

    SerialBT.println("COMECOUUUUU");
    running = true;

    switch (estrategia)
    {
    case 'a': // frentao
        forward(255, 255);
        SerialBT.print("FRENTAAO");
        delay(200);
        break;

    case 'b': // curvao
        if (direc == dir)
        {
            right(255, 255);
            delay(90);
            forward(255, 180);
            delay(500);
            left(255, 255);
            delay(200);
            direc = esq;
        }
        else
        {
            left(255, 255);
            delay(90);
            forward(180, 255);
            delay(500);
            right(255, 255);
            delay(200);
            direc = dir;
        }
        break;

    case 'c': // curvinha
        if (direc == dir)
        {
            right(255, 255);
            delay(90);
            forward(255, 90);
            delay(400);
            left(255, 255);
            delay(100);
            direc = esq;
        }
        else
        {
            left(255, 255);
            delay(90);
            forward(90, 255);
            delay(400);
            right(255, 255);
            delay(100);
            direc = dir;
        }
        break;
        
    case 'e': // costas   - só gira 180º
        if (direc == dir)
        { // prestar atenção na direc, pq os robôs estarão de costas
            right(255, 255);
            delay(190);
        }
        else
        {
            left(255, 255);
            delay(190);
        }
        break;

    case 'd': // costas   - só gira 180º
        if (direc == dir)
        { // prestar atenção na direc, pq os robôs estarão de costas
            backward(255, 90);
            delay(500);
            forward(255, 90);
            delay(600);
        }
        else
        {
            backward(90, 255);
            delay(500);
            forward(255, 255);
            delay(600);
        }
        break;

    case 'f': // vai direto pro loop
        break;

        // case 'g': // costas curvao
        //     if (direc == dir)
        //     {
        //         // move('D', 'F', 50);
        //         // move('E', 'F', 100);
        //         delay(200);
        //     }
        //     else
        //     {
        //         // move('D', 'F', 100);
        //         // move('E', 'F', 50);
        //         delay(200);
        //     }
        //     break;

    default: // CÓPIA DO FRENTÃO
        forward(255, 255);
        delay(500);
        SerialBT.print("DEFAAAAAAULT");
        break;
    }

    // stop();
    // ESP.restart();
}

void loop()
{
    if (valueSharpF>0)
    {
        while (valueSharpF>0)
        {
            forward(255, 255);
        }
    }
    else
    {
        if (valueSharpE)
            left(180, 180);
        else if (valueSharpD)
            right(180, 180);
        else if (valueSharpFd)
            forward(90, 255);
        else if (valueSharpFe)
            forward(255, 90);
        else{
          if(direc == dir){
            right(50, 50);
          }else{
            left(50, 50);
          }
        }
            
    }
}
