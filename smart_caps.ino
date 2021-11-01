/*
  name: Smart_caps revition 1
  author: José Castellanos
  date: 2021-09-01
  version: 0.2
  description: Pastillero interligente que será de ayuda en asistir tratamientos médicos. Dando fidelidad en cuanto a horarios
*/
//estructura que será usada para almacenar el objeto en la memoria eeprom
struct Medicine_str {
  uint8_t id;
  uint8_t hour_;
  uint8_t minute_;
  uint8_t inter_;

};
//Declaración de librerías 
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"
#include <EEPROM.h>
#include <Clock.h>
#include <Medicine.h>
#include <LinkedList.h>
#include <pt.h>
#include <Wire.h>

//Estados e indice para la máquina de estados
#define CONTROL       0
#define SET_HOUR_TIME 1
#define ADD_MEDICINE  2
uint8_t state = CONTROL;

//Push button para cambiar configuración, asginación de medicamentos, incremento de valores
#define BTN_MODE      0
#define BTN_INCREESE  1
#define BTN_NEXT      2

struct pt pt1; //Protohilo que monitorea los estados de cada tratamiento
//arreglo de botones que serán usados como auxilar para detectar un flaco de bajada en los push button
uint8_t button_state[3];
uint8_t button[3] = {
  5, 4, 3
};


RTC_DS1307 rtc; //objeto del módulo RTC
byte i, second, minute, hour, date, month; //variables que serán de ayuda para la configuración de los parámetros del módulo RTC
byte hour_interval = 2; //intervalo de hora para los tratamientos, estos pueden ser 2,4,6,...,24.
int year = 0; //año para ser un filtro, pues del módulo RTC proviene algo como 2020,2021, etc. el propósito es unicamente manipular el 20,21, con respecto a los bytes que un tipo "int" utiliza en memoria
LiquidCrystal_I2C lcd (0x27, 16, 2); //inicialización del objeto lcd
Clock clock_obj(lcd); //inicialización del objeto de reloj para el tratamiento
Medicine medicine_obj(lcd); //inicialización del objeto del medicamento.

unsigned long previousMilis = 0; //usado para crear un retardo/espera del sistema, sin pausar la ejecución de esta
long interval = 1000; //intervalo a esperar en ms
bool showDateHour = true; //usado para saber cuando mostrar la fehca y hora, pues al configurar los parámetros estos deben parpadear

uint8_t id_medicine; //id del tratamiento
bool saveData = true; //guardar o no guardar los datos de los medicamentos.



void notify(struct pt *pt) { //ejecución del protohilo
  PT_BEGIN(pt);
  static long current_millis = 0;
  do {
    medicine_obj.getArrayMedicines_(clock_obj.getHour(),clock_obj.getMinute()); //llama cada segundo el método encargado de revisar los estados de los tratamientos
    current_millis = millis();
    PT_WAIT_WHILE(pt, ((millis() - current_millis) < 1000)); 
  } while (true);
  PT_END(pt);
}

void blink_parameter() { //representa un espacio en el tiempo para volver a pintar el parámetro a configurar
  byte j = 0;
  while (j < 20 && digitalRead(5) && digitalRead(4)) {
    j++;
    delay(25);
  }
}
void interval_display() { //muestra el mensaje de intervalo en el lcd
  char buffer[2];
  lcd.setCursor(0, 1);
  lcd.print("INTE");
  sprintf(buffer, "%02d", hour_interval);
  lcd.setCursor(6, 1);
  lcd.print(buffer);
}
void id_display() { //muestra el mensaje del identificador del tratamiento, este no puede ser 0/cero
  lcd.setCursor(9, 1);
  lcd.print("ID:");
  lcd.print(id_medicine);
}
byte edit(uint8_t row, uint8_t column, uint8_t parameter) {//método encargado de parpadear el parámetro a modificar.
  char text[3];
  //while btn is pushed
  while (!digitalRead(5)) {
  }
  while (true) {
    if (Rising_edge(BTN_NEXT)) {
      if (state == ADD_MEDICINE) { //verifica a que estado es el proximo a pasar, esto despues de presionar el botón "NEXT"
        saveData = false;
        state = CONTROL;
        Serial.println("CONTROL");
      } else {
        state = ADD_MEDICINE;
        saveData = true;
        Serial.println("ADD_MEDICINE");
      }
      break;
    }
    while (!digitalRead(4)) {
      if (state == ADD_MEDICINE && i==7)
        parameter += 2; //si el estado está en el agregar medicamento y el intervalo a modificar es el intervalo, este debe incrementar de dos en dos
      else
        parameter++;

      if (i == 0 && parameter > 23) { //parámetro de hora del módulo RTC
        parameter = 0;
      }
      if (i == 5 && parameter > 23) {//parámetro de hora del medicamento
        parameter = 0;
      }
      if (i == 1  && parameter > 59) { //parámetro de minuto del módulo RTC
        parameter = 0;
      }
      if (i == 6 && parameter > 59) { //parámetro de minuto del tratamiento
        parameter = 0;
      }

      
      if (i == 2 && parameter > 31) {//parámetro de día del mes RTC
        parameter = 1;
      }

      if (i == 3 && parameter > 12) { //parámetro de mes del RTC
        parameter = 1;
      }

      if (i == 4 && parameter > 99) { //parámetro de año del RTC
        parameter = 0;
      }

      if (i == 7 && parameter > 24) //parámetro de intervalo de horas
        parameter = 2;

      if (i == 8 && parameter > 99) { //parámetro de id del tratamiento
        parameter = 0;
      }

      lcd.setCursor(row, column);
      if (i == 9);else {
        sprintf(text, "%02u", parameter);
        lcd.print(text);
      }
      if (i >= 10) { //si se supera los parametros a modificar vuelve a mostrar los valores ya modificados
        clock_obj.DS1307_read();
        clock_obj.DS1307_display();
        clock_obj.calendar_display();
      }
      delay(200);
    }
    lcd.setCursor(row, column);
    lcd.print("  ");
    if (i == 9) {
      lcd.print(" ");
    }
    blink_parameter();
    lcd.setCursor(row, column);
    if (i == 9) {

    } else {
      sprintf(text, "%02u", parameter);
      lcd.print(text);
    }
    blink_parameter();
    if (i >= 10) {
      clock_obj.DS1307_read();
      clock_obj.DS1307_display();
      clock_obj.calendar_display();
    }
    if ((!digitalRead(5) && i < 10) || i >= 10) {
      i++;
      showDateHour = true;
      return parameter;
    }
  }
}
/*utilitaries function*/
uint8_t Rising_edge(int btn) { //monitorea cuando existe un cambio de flancos para los push button
  uint8_t newValue = digitalRead(button[btn]);
  uint8_t result = button_state[btn] != newValue && newValue == 1;
  button_state[btn] = newValue;
  return result;
}
void mydalay() { //espera el sistema sin interrumpir la ejecución del mismo
  unsigned long currentMilis = millis();
  if ((currentMilis - previousMilis) >= interval) { //detecta cuando ya se halla transcurrido el tiempo a esperar
    previousMilis =  currentMilis;
  }
}
void callEditHour() {
  //DateTime now = rtc.now();
  i = 0; //firts parameter
  hour = edit(6, 0, clock_obj.getHour());//se envía el parametro a modificar, "i", la fila y la columna donde se encuentre este valor
  //Serial.println(hour);
  minute = edit(9, 0, clock_obj.getMinute());
  //Serial.println(minute);
  //day=edit(0,1,day);
  i = 2;
  date = edit(6, 1, clock_obj.getDate());
  //Serial.println(date);
  month = edit(9, 1, clock_obj.getMonth());
  //Serial.println(month);
  i = 4;
  year = edit(14, 1, clock_obj.getYear());
  if (state != ADD_MEDICINE) //verifica que estos atributos serán guardados siempre y cuando la agregación del tratamiento no esté en ejecución
    rtc.adjust(DateTime(year, month, date, hour, minute, second));
  delay(200);
}
void callAddMedicine() {//Función encargada de agregar el tratamiento
  showDateHour = false; //inhabilita el mostrado de la hora y fecha
  i = 0; 
  hour = edit(6, 0, clock_obj.getHour());
  minute = edit(9, 0, clock_obj.getMinute());
  i = 7;
  hour_interval = edit(6, 1, medicine_obj.getHourInterval());
  i = 8;
  id_medicine = edit(12, 1, medicine_obj.getMedicineId());
  if (saveData && hour_interval != 0 && id_medicine != 0) { //verifica si los datos se guardan y el id como el intervalo no sean 0
    Serial.println("GUARDAR DATOS");
    Medicine_str medicine = {
      id_medicine,
      hour,
      minute,
      hour_interval
    };
    medicine_obj.readAllMedicines(); //se lee la memoria eeprom para actualizar el indice de la ultima estructura guardada
    medicine_obj.addMedicine(medicine); //envía la estructura a guardar
  } else Serial.println("NO GUARDAR DATOS");
  //editMedicine();

}
void stateMachine() {
  switch (state) {
    case CONTROL:
      if (Rising_edge(BTN_MODE)) { //estado principal para configurar la hora/fecha o para agregar un tratamiento
        state = SET_HOUR_TIME;
        Serial.println("HOURTIME");
      }
      break;
    case SET_HOUR_TIME:
      lcd.setCursor(0, 0);
      lcd.clear();
      lcd.print("Conf. Hora"); //mensaje del estado
      delay(2000);
      //Visualiza los datos por si en algun otro proceso no se mostraron correctamente
      clock_obj.DS1307_read();
      clock_obj.DS1307_display();
      clock_obj.calendar_display();
      callEditHour(); //función encargada de editar los parámetros
      break;
    case ADD_MEDICINE:
      lcd.setCursor(0, 0);
      lcd.clear();
      lcd.print("Agr. Medicina"); //mensaje del estado
      delay(2000);
      lcd.clear();
      clock_obj.DS1307_read();
      clock_obj.DS1307_display();
      interval_display(); //muestra el intervalo del tratamiento
      id_display();
      callAddMedicine(); //función encargada de agregar un tratamiento
      break;


  }
}
/*void readLinkedList(LinkedList<MedicineObj*> medicineList){
  MedicineObj *obj;
  for(int i = 0; i < medicineList.size(); i++){
    obj = medicineList.get(i);
    Serial.println(obj->id);
    
  }
}*/
void setup() {
   /*for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }*/
  Serial.begin(9600);
  pinMode(button[BTN_MODE], INPUT_PULLUP);
  pinMode(button[BTN_INCREESE], INPUT_PULLUP);
  pinMode(button[BTN_NEXT], INPUT_PULLUP);
  button_state[0] = HIGH;
  button_state[1] = HIGH;
  button_state[2] = HIGH;
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  lcd.begin();
  lcd.backlight();
  medicine_obj.readAllMedicines(); //lee y visaliza en el monitor serial los medicamentos leidos, esto con proposito de debbug
  PT_INIT(&pt1);
  
  

}
void loop() {
  if (showDateHour) {
    clock_obj.DS1307_read(); //Lee los valores del módulo RTC
    clock_obj.DS1307_display(); //visualiza en el lcd la hora del modulo RTC
    clock_obj.calendar_display(); //Visaualiza en el lcd el calendario del módulo RTC
  }
  notify(&pt1); //Protohilo usado para revisar qué mediccamentos notificar
  mydalay();
  stateMachine(); //máquina de estados principal
  delay(50);

}
