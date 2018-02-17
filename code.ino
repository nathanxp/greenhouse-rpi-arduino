#include <Servo.h> 
#include <DHT22.h>
#include <SoftwareSerial.h>

#define GSM_PIN 2
#define LCD_PIN 4
#define LEDS_L_PIN 5
#define LEDS_R_PIN 8
#define LEDS_C_PIN 11 
#define SERVO_PIN 14
#define FANS_PIN 15
#define KEY_PIN 16
#define SMOKE_PIN 17
#define HUM_PIN 18
#define HUM2_PIN 19

#define LEDS_L 0
#define LEDS_R 1
#define LEDS_C 2 

#define LOWEST_TEMP_L 0
#define LOWEST_TEMP_R 1
#define HIGHEST_TEMP_L 2
#define HIGHEST_TEMP_R 3
#define LOWEST_HUM_L 4
#define LOWEST_HUM_R 5
#define HIGHEST_HUM_L 6
#define HIGHEST_HUM_R 7

#define LOWEST_TEMP_L_MESSAGE "(L) Lowest Temperature C : "
#define LOWEST_TEMP_R_MESSAGE "(R) Lowest Temperature C : "
#define HIGHEST_TEMP_L_MESSAGE "(L) Highest Temperature C : "
#define HIGHEST_TEMP_R_MESSAGE "(R) Highest Temperature C : "
#define LOWEST_HUM_L_MESSAGE "(L) Lowest Humidity % : "
#define LOWEST_HUM_R_MESSAGE "(R) Lowest Humidity % : "
#define HIGHEST_HUM_L_MESSAGE "(L) Highest Humidity % : "
#define HIGHEST_HUM_R_MESSAGE "(R) Highest Humidity % : "



byte pin_of[3] = {  
				   LEDS_L_PIN , 
				   LEDS_R_PIN , 
				   LEDS_C_PIN
				   };
				   
char* user_messages[8] = {	
						LOWEST_TEMP_L_MESSAGE, 
						LOWEST_TEMP_R_MESSAGE, 
						HIGHEST_TEMP_L_MESSAGE, 
						HIGHEST_TEMP_R_MESSAGE,
						LOWEST_HUM_L_MESSAGE, 
						LOWEST_HUM_R_MESSAGE, 
						HIGHEST_HUM_L_MESSAGE, 
						HIGHEST_HUM_R_MESSAGE	
						};
int user_limits[8];

int	damaged_sensor1 = 0,
	damaged_sensor2 = 0,
	smoke_limit = 800,
	overheating_alert = 0,
	heating_alert = 0,
	right_watering = 0,
	left_watering = 0,
	window_state = 0,
	current_l_row = 0,
	current_r_row = 0,
	current_c_row = 0,
	temp_limit = -1,
	left_temp,
	left_hum,
	right_temp,
	right_hum,
	smoke_alert = 0;

boolean leds_left[8],
		leds_right[8],
		leds_center[7];
		
char inchar,mobilenumber[] = "xxxxxxxxxx";

Servo window;
DHT22 left_sensor(HUM_PIN);
DHT22 right_sensor(HUM2_PIN);
SoftwareSerial lcd = SoftwareSerial(0, LCD_PIN);
SoftwareSerial gsm = SoftwareSerial(GSM_PIN,GSM_PIN+1);


void leds_off(int led_unit){

	int leds = 8;
	if (led_unit == LEDS_C) { leds = 7; }
   
	for(int index = 0; index <  leds; index++){ 
		if (led_unit == LEDS_L) { leds_left[index] = LOW; } 
		else if (led_unit == LEDS_R) { leds_right[index] = LOW; }
		else if (led_unit == LEDS_C) { leds_center[index] = LOW; }
	} 
  
} 

void setRegisterPin(int index, int value, int led_unit){

  if (led_unit == LEDS_L) { leds_left[index] = value; }
  else if (led_unit == LEDS_R) { leds_right[index] = value; }
  else if (led_unit == LEDS_C) { leds_center[index] = value; }

}

void toggle_leds(int led_unit){
	
	  
      int which = led_unit;
	  int leds = 8;
	  if (led_unit == LEDS_C) { leds = 7; }
	  
	  led_unit = pin_of[led_unit];
	  
	  digitalWrite(led_unit+1, LOW);

	  for(int index = 0; index <  leds; index++){
		digitalWrite(led_unit+2, LOW);
		
        int val;
		if (which == LEDS_L) { val = leds_left[index]; } 
		else if (which == LEDS_R) { val = leds_right[index]; }
		else if (which == LEDS_C) { val = leds_center[index]; }

		digitalWrite(led_unit, val);
		digitalWrite(led_unit+2, HIGH);

	  }
	  digitalWrite(led_unit+1, HIGH);
}


void lcdPosition(int row, int col) {
  lcd.write(0xFE);
  lcd.write((col + row*64 + 128)); 
  delay(10);
}
void clearLCD(){
  lcd.write(0xFE);
  lcd.write(0x01);
  delay(10);
}
void printLCD(String row1,String row2,int rows) {
  clearLCD();
  lcdPosition(0,0);
  lcd.print(row1);
  
  if (rows == 2) {
  lcdPosition(1,0);
  lcd.print(row2);
  }
}
void ask_limits() {

for (int index = 0; index < 8; index++ ) { 
	
	printLCD(user_messages[index],"",1);
	set_limit();
	user_limits[index] = temp_limit;
	temp_limit = -1;
}
	
}

void set_limit() {
	
int key = analogRead(KEY_PIN);
   	
while (key > 10) {
        
	for (int i = 0; i < 400; i++) { key = analogRead(KEY_PIN); }

	if (key == 1023) { key = 0; }
	else if (key > 910 && key < 920) { key = 1; }
	else if (key > 780 && key < 791) { key = 2; }
	else if (key > 860 && key < 870) { key = 3; }
	else if (key > 330 && key < 340) { key = 4; }
	else if (key > 990 && key <= 1000) { key = 5; }
	else if (key > 565 && key < 570) { key = 6; }
	else if (key > 470 && key < 480) { key = 7; }
	else if (key > 1005 && key <= 1010) { key = 8; }
	else if (key > 630 && key < 640) { key = 9; }
}

if (key < 10) {

	lcd.print(key);
	delay(2000);
	
	if (temp_limit == -1) { 
	
		temp_limit = key; 
		set_limit();
	} 
	else { temp_limit = (temp_limit * 10) + key; }
}
}

void watering() {

int left_status = left_sensor.readData();
int right_status = right_sensor.readData();

if (left_status == 0) {
	damaged_sensor1 = 0;
	left_temp = left_sensor.getTemperatureC();
	left_hum = left_sensor.getHumidity();
	
	if (left_hum  <  user_limits[LOWEST_HUM_L]) {
		left_watering = 1;
	}
	else if (left_hum  >=  user_limits[HIGHEST_HUM_L]) {
		left_watering = 0;
	}
	
	if (left_watering == 1) {
		leds_off(LEDS_L);
		setRegisterPin(current_l_row, HIGH , LEDS_L);
		if (current_l_row == 7) { current_l_row = 0; } else { current_l_row++; }
	} else { leds_off(LEDS_L); current_l_row = 0; }
	
	toggle_leds(LEDS_L);


} else {
damaged_sensor1 = 1;
}

if (right_status == 0) {
	damaged_sensor2 = 0;
	right_temp = right_sensor.getTemperatureC();
	right_hum = right_sensor.getHumidity();
	
	if (right_hum  <  user_limits[LOWEST_HUM_R]) {
		right_watering = 1;
	}
	else if (right_hum  >=  user_limits[HIGHEST_HUM_R]) {
		right_watering = 0;
	}
	
	if (right_watering == 1) {

		leds_off(LEDS_R);
		setRegisterPin(current_r_row, HIGH , LEDS_R);
		if (current_r_row == 7) { current_r_row = 0; } else { current_r_row++; }
		
	} else { leds_off(LEDS_R); current_r_row = 0; }
	
	toggle_leds(LEDS_R);


} else {
damaged_sensor2 = 1;
}

}


void  heating() {

	if (left_temp < user_limits[LOWEST_TEMP_L] || right_temp < user_limits[LOWEST_TEMP_R]) { 
		leds_off(LEDS_C);
		setRegisterPin(current_c_row, HIGH , LEDS_C);
		if (current_c_row == 6) { current_c_row = 0; } else { current_c_row++; }
		heating_alert = 1;
	}
	else { 
		  leds_off(LEDS_C); 
		  current_c_row = 0;
		  heating_alert = 0; 
	}
	toggle_leds(LEDS_C);
}

void smoke() {

int smoke = analogRead(SMOKE_PIN);

if (smoke > smoke_limit) { smoke_alert = 1; } else { smoke_alert = 0; }
	 
}

void overheating() {

	if (left_temp <= user_limits[HIGHEST_TEMP_L] && right_temp <= user_limits[HIGHEST_TEMP_R]) { 
	
		overheating_alert = 0;
		
	} else { overheating_alert = 1; }
}

void fans_n_window() {

if (smoke_alert == 1 || overheating_alert == 1) {
	
	window_state = 1;
	window.write(150);
	if (smoke_alert == 1) { digitalWrite(FANS_PIN, HIGH); } else { digitalWrite(FANS_PIN, LOW); }

} else {

	window_state = 0;
	window.write(0);
	digitalWrite(FANS_PIN, LOW);
	
}

}

void check_sms() {

if(gsm.available() >0)

{

inchar=gsm.read();

if (inchar=='#')

{

delay(10);

inchar=gsm.read();

if (inchar=='e')

{

sendsms();

delay(10);

gsm.println("AT+CMGD=1,4");

}

}

}
}

void sendsms() {

gsm.println("AT+CMGF=1");

gsm.print("AT+CMGS=");

gsm.print(byte(34));

gsm.print(mobilenumber);

gsm.println(byte(34));

delay(500);

String message;
if (right_watering == 1) { message += "R: watering\n"; } else { message += "R: idle\n"; }
if (left_watering == 1) { message += "L: watering\n"; } else { message += "L: idle\n"; }
if (smoke_alert == 1) { message += "Fire: yes\n"; } else { message += "Fire: no\n"; }
if (overheating_alert == 1) { message += "Hot: yes\n"; } else { message += "Hot: no\n"; }
if (heating_alert == 1) { message += "Cold: yes\n"; } else { message += "Cold: no\n"; }
if (damaged_sensor1 == 1) { message += "Sensor1: failure\n"; } else { message += "Sensor1: working\n"; }
if (damaged_sensor2 == 1) { message += "Sensor2: failure\n"; } else { message += "Sensor2: working\n"; }

gsm.print(message);

gsm.print(byte(26)); 

}

void setup(){

  pinMode(pin_of[LEDS_L], OUTPUT);
  pinMode(pin_of[LEDS_L]+1, OUTPUT);
  pinMode(pin_of[LEDS_L]+2, OUTPUT);
  
  pinMode(pin_of[LEDS_R], OUTPUT);
  pinMode(pin_of[LEDS_R]+1, OUTPUT);
  pinMode(pin_of[LEDS_R]+2, OUTPUT);
  
  pinMode(pin_of[LEDS_C], OUTPUT);
  pinMode(pin_of[LEDS_C]+1, OUTPUT);
  pinMode(pin_of[LEDS_C]+2, OUTPUT);
  
  pinMode(LCD_PIN, OUTPUT);
  pinMode(FANS_PIN, OUTPUT);
  
  window.attach(SERVO_PIN);
  
  Serial.begin(9600);
  lcd.begin(9600);

  leds_off(LEDS_L);
  leds_off(LEDS_R);
  leds_off(LEDS_C);
  toggle_leds(LEDS_L);
  toggle_leds(LEDS_R);
  toggle_leds(LEDS_C);
  
  gsm.begin(9600);
  gsm.println("AT+CMGF=1");
  gsm.println("AT+CNMI=3,3,0,0");
  
  printLCD("Welcome to","Green House",2);
  
  delay(3000);
  
  ask_limits();
}

void loop(){

	printLCD("Operating.","",1);
	delay(500);
	printLCD("Operating..","",1);
	delay(500);
	printLCD("Operating...","",1);
	delay(500);

	
	smoke();
	overheating();
	fans_n_window();
	heating();
	watering();
	check_sms();
	
	Serial.println(String(left_temp)+","+String(left_hum)+","+String(right_temp)+","+String(right_hum)+","+String(heating_alert)+","+String(overheating_alert)+","+String(smoke_alert)+","+String(left_watering)+","+String(right_watering)+","+String(user_limits[LOWEST_TEMP_L])+","+String(user_limits[HIGHEST_TEMP_L])+","+String(user_limits[LOWEST_TEMP_R])+","+String(user_limits[HIGHEST_TEMP_R])+","+String(user_limits[LOWEST_HUM_L])+","+String(user_limits[HIGHEST_HUM_L])+","+String(user_limits[LOWEST_HUM_R])+","+String(user_limits[HIGHEST_HUM_R]));

}
