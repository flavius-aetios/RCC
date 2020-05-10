#include <SoftwareSerial.h>
SoftwareSerial mySerial(2, 3); //RX pin, TX pin

#define P1 12     //Реле Р1
#define P2 13     //Реле Р2

// Период таймера В МИЛЛИСЕКУНДАХ
// дней*(24 часов в сутках)*(60 минут в часе)*(60 секунд в минуте)*(1000 миллисекунд в секунде)
unsigned long period_time = (long)65*60*1000;  //65 минут
unsigned long my_timer;

void setup() {
  delay(2000);  
  pinMode(P1, OUTPUT);
  pinMode(P2, OUTPUT);
  digitalWrite(P1, LOW);
  digitalWrite(P2, LOW);

  mySerial.begin(9600);   // Установка скорости передачи данных GSM Module  
  Serial.begin(9600);     // Установка скорости передачи данных Serial Monitor (Arduino)
  delay(5000);

  Serial.println("Serial ports ready!");
  Serial.println();
  
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
