/* WiFi STA Telegram */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <UniversalTelegramBot.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include <time.h>
#include <EEPROM.h>
#include "secrets.h"
#include "html_page.h"
#define RELAY_1 D5
#define RELAY_2 D6
#define EEPROM_ADDR 0

volatile bool flag_i = false;
volatile bool flag_ip = false;
volatile bool flag_status = false;
bool timer1_on_active = false;
bool timer2_on_active = false;
bool timer1_off_active = false;
bool timer2_off_active = false;
bool randomize_enabled = false;
bool daily_random_done = false;
volatile uint32_t clock_sec = 0;
uint32_t timer1_off_target = 0;
uint32_t timer2_off_target = 0;
uint32_t timer1_on_target = 0;
uint32_t timer2_on_target = 0;
uint32_t timer1_on_random = 0;
uint32_t timer1_off_random = 0;
uint32_t timer2_on_random = 0;
uint32_t timer2_off_random = 0;
volatile uint16_t count = 0;
volatile uint8_t clock_div = 0;
uint8_t t_s = 100;
String chat_id  = "";
String IP = "";
String last_cmd;
String pend_cmd;
String last_msg;
Ticker timer;

// Telegram client
WiFiClientSecure client_telegram;
UniversalTelegramBot bot(BOT_TOKEN, client_telegram);

// MQTT client
WiFiClientSecure client_mqtt;
PubSubClient mqtt(client_mqtt);

// WebServer
ESP8266WebServer server(80);



// --------------------------
// webserver
// --------------------------
void handleRoot(){
  server.send_P(200, "text/html", HTML_page);
}

void handleWebCommand() {
  String cmd = server.arg("c");

  if (cmd == "STATUS") {
    server.send(200, "text/plain", statusRelays());
    return;
  }
}



// --------------------------
// MQTT
// --------------------------
void reconnectMQTT() {
  while (!mqtt.connected()) {
    if (mqtt.connect("NodeMCU_Client", MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("[MQTT] Connected. ✔");
      mqtt.subscribe("esp8266/cmd");
      publishStatus();
    } else {
      Serial.print("[MQTT] Failed to connect. ❌");
      delay(t_s*1000);
    }
  }
}

void publishStatus() {
  String msg = "{";
  msg += "\"relay1\":" +String(digitalRead(RELAY_1) == LOW ? 1 : 0) +",";
  msg += "\"relay2\":" +String(digitalRead(RELAY_2) == LOW ? 1 : 0) +",";
  msg += "\"t1_on\":\""  +formatTime(timer1_on_target)  +"\",";
  msg += "\"t2_on\":\""  +formatTime(timer2_on_target)  +"\",";
  msg += "\"t1_off\":\"" +formatTime(timer1_off_target) +"\",";
  msg += "\"t2_off\":\"" +formatTime(timer2_off_target) +"\"";
  msg += ",\"timer1_on_active\":"  +String(timer1_on_active ? 1 : 0);
  msg += ",\"timer1_off_active\":" +String(timer1_off_active ? 1 : 0);
  msg += ",\"timer2_on_active\":"  +String(timer2_on_active ? 1 : 0);
  msg += ",\"timer2_off_active\":" +String(timer2_off_active ? 1 : 0);
  if (randomize_enabled && daily_random_done) {
    msg += ",\"t1_on_random\":\""  +formatTime(timer1_on_random)  +"\",";
    msg += "\"t1_off_random\":\"" +formatTime(timer1_off_random) +"\",";
    msg += "\"t2_on_random\":\""  +formatTime(timer2_on_random)  +"\",";
    msg += "\"t2_off_random\":\"" +formatTime(timer2_off_random) +"\"";
  }
  msg += "}";
  mqtt.publish("esp8266/status", msg.c_str());
}

void check_mqtt(char* topic, byte* payload, unsigned int length) {
  String cmd;
  for (unsigned int i=0; i<length; i++) cmd += (char)payload[i];
  handleCommand(cmd);  
}



// --------------------------
// Telegram
// --------------------------
void send_ReplyKeyboard() {
  String keyboard ="[[\"POWER ON 1\",\"POWER ON 2\"],[\"POWER OFF 1\",\"POWER OFF 2\"],[\"SET T1\",\"SET T2\"],[\"TOOGLE T1\",\"TOOGLE T2\"],[\"TOOGLE RANDOM\",\"STATUS\"]]";
  bot.sendMessageWithReplyKeyboard(chat_id, "Control panel", "", keyboard, true, false, false);
}

void telegram(){
  uint8_t num = bot.getUpdates(bot.last_message_received +1);
  if (last_msg.length() > 0) { bot.sendMessage(chat_id, last_msg, ""); last_msg = ""; }
  if (num == 0) return;
  
  for (uint8_t i = 0; i < num; i++) {
    chat_id    = bot.messages[i].chat_id;
    String cmd = bot.messages[i].text;

    if (cmd == "/start") send_ReplyKeyboard();
    if (!flag_ip) { bot.sendMessage(chat_id, "WiFi connected IP " +IP, ""); flag_ip = true; }
    if (cmd == "SET T1" || cmd == "SET T2") { bot.sendMessage(chat_id, "Provide start time in hh:mm", ""); last_cmd = cmd +" ON"; }
    else if (last_cmd.endsWith("ON")) { bot.sendMessage(chat_id, "Provide stop time in hh:mm", ""); pend_cmd = last_cmd +" " +cmd; last_cmd.replace("ON", "OFF"); }    
    else if (last_cmd.endsWith("OFF")) { pend_cmd = last_cmd + " " + cmd; last_cmd = ""; flag_status = true; }
    if (pend_cmd.length() > 0) { cmd = pend_cmd; pend_cmd = ""; };
    handleCommand(cmd);
  }
}



// --------------------------
// Timers
// --------------------------
void setTimer(bool &active, uint32_t &target, uint32_t amount_sec) {
  target = amount_sec;
  active = true;
  saveTargets();
  if (flag_status) { flag_status = false; statusRelays(); }
}

void stopTimer(bool &active) {
  active = false;
  saveTargets();
  statusRelays();
}

void checkTimer(bool &active, uint32_t &target, void (*action)(), const char* msg) {
  if (!active) return;

  uint32_t realTarget = target;
  if (randomize_enabled){
    if (action == activateRelay1 && target == timer1_on_target) realTarget = timer1_on_random;
    if (action == disableRelay1 && target == timer1_off_target) realTarget = timer1_off_random;
    if (action == activateRelay2 && target == timer2_on_target) realTarget = timer2_on_random;
    if (action == disableRelay2 && target == timer2_off_target) realTarget = timer2_off_random;    
  }

  if (clock_sec >= realTarget && clock_sec < realTarget+15) {
    action();
  }
}

uint32_t randomBetween(int32_t a, int32_t b) {
  if (a < 0) a = 0;
  if (b > 86399) b = 86399;
  if (b <= a) return a;  
  return random(a, b);
}

void randomizeTimer() {
  if (!randomize_enabled) return;
  if (daily_random_done) return;

  if (timer1_on_target && timer1_off_target) {
    uint32_t delta = min(uint32_t(3600), (timer1_off_target -timer1_on_target)/2);
    timer1_on_random  = randomBetween(timer1_on_target, timer1_on_target+delta);
    timer1_off_random = randomBetween(timer1_off_target-delta, timer1_off_target);
  }
  if (timer2_on_target && timer2_off_target) {
    uint32_t delta = min(uint32_t(3600), (timer2_off_target -timer2_on_target)/2);
    timer2_on_random  = randomBetween(timer2_on_target, timer2_on_target+delta);
    timer2_off_random = randomBetween(timer2_off_target-delta, timer2_off_target);
  }
  daily_random_done = true;
}

String formatTime(uint32_t target) {
  if (!target) return "--:--";

  uint32_t h = target/3600;
  uint32_t m = (target%3600)/60;

  char buffer[6];
  sprintf(buffer, "%02u:%02u", h, m);
  return String(buffer);
}

bool parseHHMM(const String &s, uint32_t &outSec) {
  if (s.length() != 5) return false;
  if (s.charAt(2) != ':') return false;

  int hh = (s.charAt(0)-'0')*10 +(s.charAt(1)-'0');
  int mm = (s.charAt(3)-'0')*10 +(s.charAt(4)-'0');
  if (hh < 0 || hh > 23 || mm < 0 || mm > 59) return false;

  outSec = hh*3600 + mm*60;
  return true;
}

void handleCommand(String cmd) {
  cmd.trim();   

  // ====== MAIN CONTROL ======
  if (cmd == "POWER ON 1")  activateRelay1();
  if (cmd == "POWER ON 2")  activateRelay2();
  if (cmd == "POWER OFF 1") disableRelay1();
  if (cmd == "POWER OFF 2") disableRelay2();
  if (cmd == "STATUS") statusRelays();

  // ====== SET TIMERS ======
  // SET T1 ON 00:00
  if (cmd.startsWith("SET ")) {
    int p1 = cmd.indexOf(' ');
    int p2 = cmd.indexOf(' ', p1+1);
    int p3 = cmd.indexOf(' ', p2+1);

    String timer = cmd.substring(p1+1, p2);   // T1 / T2
    String mode  = cmd.substring(p2+1, p3);   // ON / OFF
    String hm    = cmd.substring(p3+1);       // hh:mm

    uint32_t sec;
    if (!parseHHMM(hm, sec)) return;
    if (timer=="T1" && mode=="ON") setTimer(timer1_on_active, timer1_on_target, sec);
    if (timer=="T1" && mode=="OFF") setTimer(timer1_off_active, timer1_off_target, sec);
    if (timer=="T2" && mode=="ON") setTimer(timer2_on_active, timer2_on_target, sec);
    if (timer=="T2" && mode=="OFF") setTimer(timer2_off_active, timer2_off_target, sec);
  }

  // ====== STOP TIMERS ======
  if (cmd == "TOOGLE T1"){
    bool anyActive = timer1_on_active || timer1_off_active;
    if (anyActive){ stopTimer(timer1_on_active); stopTimer(timer1_off_active); statusRelays(); }
    else { timer1_on_active  = (timer1_on_target > 0); timer1_off_active = (timer1_off_target > 0); daily_random_done = false; statusRelays(); }
  }
  if (cmd == "TOOGLE T2"){
    bool anyActive = timer2_on_active || timer2_off_active;
    if (anyActive){ stopTimer(timer2_on_active); stopTimer(timer2_off_active); statusRelays(); }
    else { timer2_on_active  = (timer2_on_target > 0); timer2_off_active = (timer2_off_target > 0); daily_random_done = false; statusRelays(); }
  }
  if (cmd == "TOOGLE RANDOM"){
    randomize_enabled = !randomize_enabled;
    daily_random_done = false;
    if(randomize_enabled) randomizeTimer();
    statusRelays();
  }
  publishStatus();
}



// --------------------------
// Relays
// --------------------------
void activateRelay1() { digitalWrite(RELAY_1, LOW); last_msg = "Relay 1 ON"; }
void activateRelay2() { digitalWrite(RELAY_2, LOW); last_msg = "Relay 2 ON"; }
void disableRelay1() { digitalWrite(RELAY_1, HIGH); last_msg = "Relay 1 OFF"; }
void disableRelay2() { digitalWrite(RELAY_2, HIGH); last_msg = "Relay 2 OFF"; }
String statusRelays() {
  String msg;
  msg  = "Relay 1 "   +String(digitalRead(RELAY_1) == LOW ? "ON" : "OFF") +" | Timer 1 " +String((timer1_on_active || timer1_off_active) ? "ON " : "OFF ") +formatTime(timer1_on_target) +"-" +formatTime(timer1_off_target);
  if (randomize_enabled && daily_random_done) { msg += " | Random " +formatTime(timer1_on_random) +"-" +formatTime(timer1_off_random); }
  msg += "\nRelay 2 " +String(digitalRead(RELAY_2) == LOW ? "ON" : "OFF") +" | Timer 2 " +String((timer2_on_active || timer2_off_active) ? "ON " : "OFF ") +formatTime(timer2_on_target) +"-" +formatTime(timer2_off_target);
  if (randomize_enabled && daily_random_done) { msg += " | Random " +formatTime(timer2_on_random) +"-" +formatTime(timer2_off_random); }
  last_msg = msg;
  return msg;
}



// --------------------------
// Clock and EEPROM
// --------------------------
void syncClock() {
  // UTC-3
  configTime(-3*3600, 0, "pool.ntp.org");

  time_t now = time(nullptr);
  while (now < 100000) {  
    delay(200);
    now = time(nullptr);
  }

  struct tm*t = localtime(&now);
  clock_sec = t->tm_hour*3600 +t->tm_min*60 +t->tm_sec;
  //Serial.println("[Clock] NTP synchronized. ✔");
}

void ICACHE_RAM_ATTR Timer_ISR() {
  flag_i = true;

  clock_div++;
  if (clock_div >= 10){
    clock_div = 0;
    clock_sec ++;
  }
  if(clock_sec >= 86400) { clock_sec = 0; daily_random_done = false; };
}

void saveTargets() {
  EEPROM.put(0,  timer1_on_target);
  EEPROM.put(4,  timer2_on_target);
  EEPROM.put(8,  timer1_off_target);
  EEPROM.put(12, timer2_off_target);
  EEPROM.put(16, timer1_on_active);
  EEPROM.put(17, timer2_on_active);
  EEPROM.put(18, timer1_off_active);
  EEPROM.put(19, timer2_off_active);
  EEPROM.commit();
}

void loadTargets() {
  EEPROM.get(0,  timer1_on_target);
  EEPROM.get(4,  timer2_on_target);
  EEPROM.get(8,  timer1_off_target);
  EEPROM.get(12, timer2_off_target);
  EEPROM.get(16, timer1_on_active);
  EEPROM.get(17, timer2_on_active);
  EEPROM.get(18, timer1_off_active);
  EEPROM.get(19, timer2_off_active);
}



// --------------------------
// Setup and Main
// --------------------------
void setup() {
  EEPROM.begin(64);
  loadTargets();

  Serial.begin(115200);
  timer.attach_ms(t_s, Timer_ISR);
	
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  digitalWrite(RELAY_1, HIGH);
  digitalWrite(RELAY_2, HIGH);

  // connect to WiFi
  WiFi.begin(HOME_SSID, HOME_PASSWORD);
  WiFi.setAutoReconnect(true);
  while (WiFi.status() != WL_CONNECTED){
    delay(1000);
  }
  IP = WiFi.localIP().toString();
  Serial.println("[WiFi] Connected IP " +IP +".✔");
  
  // MQTT client secure
  client_mqtt.setInsecure();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(check_mqtt);

  // webserver
  server.on("/", handleRoot);
  server.on("/cmd", handleWebCommand);
  server.begin();
  Serial.println("[WebServer] Started. ✔");

  // Telegram client
  client_telegram.setInsecure();
  client_telegram.setBufferSizes(4096, 4096);
  if (client_telegram.connect("api.telegram.org", 443)) {
    Serial.println("[Telegram] Connected. ✔");
    bot.getUpdates(-1);
    bot.last_message_received = 0;
  } else {
    Serial.println("[Telegram] Failed to connect. ❌");
  }

  // NTP time
  syncClock();
}

void loop() {
  // MQTT
  mqtt.loop();

  // webserver
  server.handleClient();

  if (flag_i) {
    flag_i = false;
    count ++;

    if (count >= 20){
      count = 0;
      if (!mqtt.connected()) reconnectMQTT();
      if (!client_telegram.connected()) client_telegram.connect("api.telegram.org", 443);
      if (!daily_random_done) randomizeTimer();

      // check timers
      checkTimer(timer1_off_active, timer1_off_target, disableRelay1, "Timer 1 OFF finished");
      checkTimer(timer2_off_active, timer2_off_target, disableRelay2, "Timer 2 OFF finished");
      checkTimer(timer1_on_active, timer1_on_target, activateRelay1, "Timer 1 ON finished");
      checkTimer(timer2_on_active, timer2_on_target, activateRelay2, "Timer 2 ON finished");

      // telegram
      telegram();
    }
  }
}