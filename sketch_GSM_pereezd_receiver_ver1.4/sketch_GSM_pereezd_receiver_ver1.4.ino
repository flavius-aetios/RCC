/*******************************************************************************
           
* File              : sketch_GSM_pereezd_receiver_ver1.x.ino
* Last modified     : 11.05.2020
* Version           : 1.4
* 
* Author            : Zaytsev Mikhail
* Support mail      : mihail25.98@gmail.com
* 
* Target MCU        : Arduino UNO
* Description       : Программа для приёма СМС от системы контроля на переезде  
*                     и для переключения состояний реле для последующей индикации
* 
********************************************************************************/

#include <SoftwareSerial.h>

// ***************************** НАСТРОЙКИ *****************************

// ****************** ВПИСАТЬ НОМЕР ПЕРЕДАТЧИКА НА ПЕРЕЕЗДЕ ************
const String TRANSMITTER_NUMBER = "+79992221155";

//------ настройка пинов подключения GSM-модуля
SoftwareSerial mySerial(8, 9); //RX pin, TX pin

//------ настройка пинов подключения реле
#define P1 12     //Реле Р1 - 12 пин
#define P2 13     //Реле Р2 - 13 пин

//------ настройка периода отправки сообщений
#define PERIOD_TIME 70      // 70 минут - интервал, в течение которого ожидается сообщение "GSM_OK". 
                            // Если такое сообщение не пришло, обесточиваются реле Р1 и Р2, на табло: "Б" - мигает 

// ************************** ДЛЯ РАЗРАБОТЧИКОВ ***********************

// дней*(24 часов в сутках)*(60 минут в часе)*(60 секунд в минуте)*(1000 миллисекунд в секунде)
unsigned long period_time = (long)PERIOD_TIME*60*1000;  //70 минут
unsigned long my_timer;

void SendMessage(String msg){
  mySerial.println("AT+CMGF=1");    //Установка GSM модуля в Text Mode
  delay(1000);  // Ожидаем 1 секунду

  mySerial.println("AT+CMGS=\"" + TRANSMITTER_NUMBER + "\"\r"); // Номер телефона КУДА отправлять
  delay(1000);

  mySerial.println(msg);// Текст SMS, который хотим отправить
  delay(100); 
  
  mySerial.println((char)26);// ASCII код комбинации CTRL+Z
  Serial.println("Send: RECEIVER_READY"); 
  delay(1000);
}

void setup() {
    
  pinMode(P1, OUTPUT);
  pinMode(P2, OUTPUT);
  digitalWrite(P1, LOW);
  digitalWrite(P2, HIGH);

  mySerial.begin(9600);   // Установка скорости передачи данных GSM Module  
  Serial.begin(9600);     // Установка скорости передачи данных Serial Monitor (Arduino)
  delay(15000);           // Ожидаем 15 секунд, чтобы GSM модуль успел запуститься

  Serial.println("Serial ports ready!");
  Serial.println();
  
  String msg = "";
  
  while (1){
    Serial.println("Проверка включения GSM модуля...");
    mySerial.println("AT");        //Даём модулю команту "AT"
    delay(300);
  
    char ch = ' ';
    String msg = "";
  
    while(mySerial.available()) {  
      ch = mySerial.read();
      msg += char(ch);   //собираем принятые символы в строку
      delay(3);
      }
  
     Serial.println(msg);
     Serial.println(msg.indexOf("OK"));
  
     if(msg.indexOf("OK") > -1) break;  //Если в ответе от модуля содержится "OK", выходим из цикла
  }
  delay(1000);
  
  Serial.println("Turn on AOH:");
  mySerial.println("AT+CLIP=1");  //Включить АОН
  delay(300);
  
  Serial.println("Text format sms:");
  mySerial.println("AT+CMGF=1");  // Текстовый формат SMS
  delay(300);
  
  Serial.println("Mode GSM:");
  mySerial.println("AT+CSCS=\"GSM\"");  // Формат кодировка текста - GSM
  delay(300);
  
  Serial.println("SMS to terminal:");
  mySerial.println("AT+CNMI=2,2,0,0,0"); // Новые SMS сразу выводятся и НЕ сохраняются на симке
  delay(300);

  SendMessage("RECEIVER_READY");
  
  my_timer = millis();
}

void loop() {
 if(mySerial.available()) //Если от модуля что-то пришло
 {  
    char ch = ' ';
    String msg = "";

    while(mySerial.available()) 
    {  
       ch = mySerial.read();
       msg += char(ch);   //собираем принятые символы в строку
       delay(3);
    }

    Serial.print("Neo send> ");
    Serial.println(msg);
    
    if(msg.indexOf("+CMT") > -1) //если есть входящее sms
    { 
      if(msg.indexOf("BREAKING") > -1) // Пришла команда "BREAKING"?
      {  
         Serial.println("BREAKING!");
         digitalWrite(P1, HIGH);    //Неисправность "К" лампа горит - С
         digitalWrite(P2, LOW);     //
         my_timer = millis();       //Сброс таймера
      }

      if(msg.indexOf("ALARM") > -1) // Пришла команда "ALARM"?
      {  
         Serial.println("ALARM!");
         digitalWrite(P1, LOW);     //Авария. "К" лампа мигает - СМ
         digitalWrite(P2, LOW);     //
         my_timer = millis();       //Сброс таймера
      }

      if(msg.indexOf("GSM_OK") > -1) // Пришла команда "GSM_OK"?
      { 
         Serial.println("GSM_OK!");
         digitalWrite(P1, HIGH);    //Всё исправно. "Б" лампа горит - С 
         digitalWrite(P2, HIGH);    // 
         my_timer = millis();       //Сброс таймера
      }
    } 
  }
  if(millis() - my_timer >= period_time){
    Serial.println("GSM_DOWN!");
    digitalWrite(P1, LOW);    //Неисправность системы контроля. "Б" лампа мигает - СМ 
    digitalWrite(P2, HIGH);   //
  }

  Serial.println("End of cycle...");
  delay(2000);
}
