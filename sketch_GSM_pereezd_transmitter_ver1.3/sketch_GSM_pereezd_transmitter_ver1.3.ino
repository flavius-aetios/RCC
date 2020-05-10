#include <SoftwareSerial.h>
SoftwareSerial mySerial(8, 9); //RX pin, TX pin 

#define PIN_ALARM 4        //Определение пина для цепи "Авария"
#define PIN_BREAKING 5     //Определение пина для цепи "Неисправность"

bool alarm = false;         //Вспомогательный флаг. "Авария - true", "Нет аварии - false" 
bool breaking = false;      //Вспомогательный флаг. "Неисправность - true", "Нет неисправности - false"

// Период таймера В МИЛЛИСЕКУНДАХ
// дней*(24 часов в сутках)*(60 минут в часе)*(60 секунд в минуте)*(1000 миллисекунд в секунде)
//unsigned long period_time = (long)60*60*1000;  //60 минут
//unsigned long period_time_30_min = (long)30*60*1000;  //60 минут

unsigned long period_time = (long)1*30*1000;  //60 минут
unsigned long period_time_30_min = (long)1*15*1000;  //60 минут

unsigned long my_timer;

void SendMessage(String msg){
  mySerial.println("AT+CMGF=1");    //Установка GSM модуля в Text Mode
  delay(1000);  // Ожидаем 1 секунду
  
  mySerial.println("AT+CMGS=\"+79234214237\"\r"); // Номер телефона КУДА отправлять
  delay(1000);
  
  mySerial.println(msg);// Текст SMS, который хотим отправить
  delay(100); 
  
  mySerial.println((char)26);// ASCII код комбинации CTRL+Z
  delay(1000);
}

void setup() {
  delay(2000);
  pinMode(PIN_ALARM, INPUT_PULLUP);    //Установка состояния пина в режим входа с подтяжкой к питанию
  pinMode(PIN_BREAKING, INPUT_PULLUP);
  
  mySerial.begin(9600);   // Установка скорости передачи данных GSM Module  
  Serial.begin(9600);    // Установка скорости передачи данных Serial Monitor (Arduino)
  delay(1000);

  my_timer = millis();

  //TODO: Ждём готовность GSM/GPRS к работе:

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
    Serial.println("Breaking!"); //Для отладки
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
  
  if((digitalRead(PIN_ALARM) && alarm) && (millis() - my_timer > period_time_30_min)){
  Serial.println("Alarm!");     //Для отладки
    SendMessage("ALARM");       //Отправляем "ALARM", если авария осталась спустя 30 минут
    my_timer = millis();        //Сброс таймера
    delay(5000);
  }
  else if((digitalRead(PIN_BREAKING) && breaking) && (millis() - my_timer > period_time_30_min)){
	Serial.println("Breaking!");  //Для отладки
    SendMessage("BREAKING");    //Отправляем "BREAKING", если неисправность осталась спустя 30 минут
	  my_timer = millis();        //Сброс таймера
    delay(5000);
  }
  

  //Крутимся в цикле пока не прошёл один час и не было неисправности или аварии
  while((millis() - my_timer <= period_time) && !(breaking || alarm)){
    Serial.println("Hour cycle..."); //Для отладки
    delay(1000);
    if(digitalRead(PIN_BREAKING)){   //Условие выполняется, если возникла неисправность
      Serial.println("Breaking in cycle!"); //Для отладки
      SendMessage("BREAKING");                  //Отправляем "BREAKING"
      breaking = true;
	    my_timer = millis();    					//Сброс таймера
      break;                                    //Выходим из цикла
    }
    if(digitalRead(PIN_ALARM)){
      Serial.println("Alarm in cycle!"); //Для отладки
      SendMessage("ALARM");
      alarm = true;
      //breaking = true;
	    my_timer = millis();    			//Сброс таймера
      break;
    } 
  }
  
  Serial.println("End of global cycle..."); //Для отладки
  
  delay(2000);
}
