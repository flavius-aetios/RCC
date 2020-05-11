/*******************************************************************************
           
* File              : sketch_GSM_pereezd_transmitter_ver1.x.ino
* Last modified     : 11.05.2020
* Version           : 1.4
* 
* Author            : Zaytsev Mikhail
* Support mail      : mihail25.98@gmail.com
* 
* Target MCU        : Arduino UNO
* Description       : Программа для отправки СМС с информацией о состоянии цепей на переезде
* 
********************************************************************************/

#include <SoftwareSerial.h>

// ***************************** НАСТРОЙКИ *****************************

//------ настройка пинов подключения GSM-модуля
SoftwareSerial mySerial(8, 9); //RX pin, TX pin 

//------ настройка пинов подключения к цепям
#define PIN_ALARM 4        //Пин для цепи "Авария" - 4
#define PIN_BREAKING 5     //Пин для цепи "Неисправность" - 5

// ****************** ВПИСАТЬ НОМЕР ПРИЁМНОГО УСТРОЙСТВА ***************
const String RECEIVER_NUMBER = "+79991117788";

// ************* ВПИСАТЬ НОМЕР ТЕЛЕФОНА, на который будут отсылаться *************
// сообщения ТОЛЬКО об АВАРИИ или НЕИСПРАВНОСТИ каждые 30 минут ******************
// ********* Оставить кавычки пустыми, если нет необходимости отправлять на второй номер **
const String SECOND_NUMBER = "+79992227788";

//------ настройка периода таймера В МИЛЛИСЕКУНДАХ
// дней*(24 часов в сутках)*(60 минут в часе)*(60 секунд в минуте)*(1000 миллисекунд в секунде)
unsigned long period_time = (long)60*60*1000;  //60 минут - период отправки сообщения о штатном режиме работы системы "GSM_OK" 

unsigned long period_time_30_min = (long)30*60*1000;  //30 минут - период отправки сообщения при неисправности или аварии 
                                                      //(должно быть МЕНЬШЕ периода отправки "GSM_OK")
unsigned long my_timer;

// ************************** ДЛЯ РАЗРАБОТЧИКОВ ***********************

bool alarm = false;         //Вспомогательный флаг. "Авария - true", "Нет аварии - false" 
bool breaking = false;      //Вспомогательный флаг. "Неисправность - true", "Нет неисправности - false"

void SendMessage(String msg){
  mySerial.println("AT+CMGF=1");    //Установка GSM модуля в Text Mode
  delay(1000);  // Ожидаем 1 секунду

  mySerial.println("AT+CMGS=\"" + RECEIVER_NUMBER + "\"\r"); // Номер телефона КУДА отправлять
  delay(1000);

  mySerial.println(msg);// Текст SMS, который хотим отправить
  delay(100); 
  
  mySerial.println((char)26);// ASCII код комбинации CTRL+Z

  //Если надо отправить сообщение об аварии или неисправности
  if (!msg.equals("GSM_OK")){
    delay(10000); // Задержка 10 секунд перед отправкой сообщения на второй номер
    
    mySerial.println("AT+CMGF=1");    //Установка GSM модуля в Text Mode
    delay(300);  // Ожидаем 1 секунду
  
    mySerial.println("AT+CMGS=\"" + SECOND_NUMBER + "\"\r"); // Номер телефона КУДА отправлять
    delay(300);
  
    if(msg.equals("ALARM")){
      Serial.println("Send secong msg...");   //Для отладки
      mySerial.println("Vnimanie! AVARIYA na pereezde Antibesskoy!");// Текст SMS, который хотим отправить
      delay(100);
    }
    else if(msg.equals("BREAKING")){
      Serial.println("Send secong msg...");   //Для отладки
      mySerial.println("Vnimanie! NEISPRAVNOST na pereezde Antibesskoy!");// Текст SMS, который хотим отправить
      delay(100);
    } 
    
    mySerial.println((char)26);// ASCII код комбинации CTRL+Z
    delay(1000);
  }
}

void setup() {

  pinMode(PIN_ALARM, INPUT_PULLUP);    //Установка состояния пина в режим входа с подтяжкой к питанию
  pinMode(PIN_BREAKING, INPUT_PULLUP);
  
  mySerial.begin(9600);   // Установка скорости передачи данных GSM Module  
  Serial.begin(9600);     // Установка скорости передачи данных Serial Monitor (Arduino)
  
  delay(15000);           // Ожидаем 15 секунд, чтобы GSM модуль успел запуститься

  //Проверка на включение GSM модуля
  String msg = "";
  while (1){
    Serial.println("Проверка включения GSM модуля...");
    mySerial.println("AT");         //Даём модулю команту "AT"
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

  //Настройки для получения СМС
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

  
  //Крутимся в цикле, пока не придёт сообщение о готовности приёмника "RECEIVER_READY"
  while(1){
    if(mySerial.available()) //Если от модуля что-то пришло
    {
      char ch = ' ';
      String msg = "";

      while(mySerial.available()) {  
         ch = mySerial.read();
         msg += char(ch);   //собираем принятые символы в строку
         delay(3);
      }
  
      Serial.print("Base send> ");
      Serial.println(msg);
      
      if(msg.indexOf("+CMT") > -1) //если есть входящее sms
      { 
        if(msg.indexOf("RECEIVER_READY") > -1) // Пришла команда "RECEIVER_READY"?
        {  
           Serial.println("RECEIVER_READY!");
           break;
        }
      }
    }  
  }
  
  my_timer = millis();    // "Сброс" таймера
}

void loop() {
  
  //Если нет неисправности и нет аварии или исправили текущую неисправность
  if((((!digitalRead(PIN_BREAKING)  && !digitalRead(PIN_ALARM)) || 
    (!digitalRead(PIN_BREAKING)   && breaking))) &&
    !(digitalRead(PIN_ALARM) && alarm)){
    
    SendMessage("GSM_OK");    //Отправляем, что всё хорошо
    Serial.println("GSM_OK"); //Для отладки
    alarm = false;
    breaking = false;
	  my_timer = millis();      //Сброс таймера
    delay(5000);
  }
  //Иначе, если исправили текущую аварию
  else if(!digitalRead(PIN_ALARM) && alarm){
    SendMessage("BREAKING");      //Отправляем, что есть неисправность
    Serial.println("Breaking!");  //Для отладки
    alarm = false;
    breaking = true;
    my_timer = millis();    //Сброс таймера
	  delay(5000);
  }
  //Иначе, если есть неисправность или авария
  else{
    if(digitalRead(PIN_BREAKING) && !breaking && !digitalRead(PIN_ALARM)){   
      Serial.println("Breaking!");  //Для отладки
      SendMessage("BREAKING");      //Отправляем "BREAKING", если есть неисправность
      breaking = true;
	    my_timer = millis();    //Сброс таймера
      delay(5000);
    }
    if(digitalRead(PIN_ALARM) && !alarm){
      Serial.println("Alarm!");  //Для отладки
      SendMessage("ALARM");      //Отправляем "ALARM", если есть авария
      alarm = true;
      //breaking = true;
	    my_timer = millis();       //Сброс таймера
      delay(5000);
    }
  }

  //Если аварию не устранили в течение 30 минут
  if((digitalRead(PIN_ALARM) && alarm) && (millis() - my_timer > period_time_30_min)){
    Serial.println("Alarm!");     //Для отладки
    SendMessage("ALARM");         //Отправляем "ALARM", если авария осталась спустя 30 минут
    my_timer = millis();          //Сброс таймера
    delay(5000);
  }
  //Иначе, если неисправность не устранили в течение 30 минут
  else if((digitalRead(PIN_BREAKING) && breaking) && (millis() - my_timer > period_time_30_min)){
	  Serial.println("Breaking!");  //Для отладки
    SendMessage("BREAKING");      //Отправляем "BREAKING", если неисправность осталась спустя 30 минут
	  my_timer = millis();          //Сброс таймера
    delay(5000);
  }
  
  //Крутимся в цикле пока не прошёл один час и не было неисправности или аварии
  while((millis() - my_timer <= period_time) && !(breaking || alarm)){
    
    Serial.println("Hour cycle..."); //Для отладки
    delay(1000);
    
    if(digitalRead(PIN_BREAKING)){   //Условие выполняется, если возникла неисправность
      Serial.println("Breaking in cycle!"); //Для отладки
      SendMessage("BREAKING");              //Отправляем "BREAKING"
      breaking = true;
	    my_timer = millis();    					//Сброс таймера
      break;                            //Выходим из цикла
    }
    if(digitalRead(PIN_ALARM)){
      Serial.println("Alarm in cycle!"); //Для отладки
      SendMessage("ALARM");
      alarm = true;
      //breaking = true;
	    my_timer = millis();    			    //Сброс таймера
      break;
    } 
  }
  
  Serial.println("End of global cycle..."); //Для отладки
  
  delay(2000);
}
