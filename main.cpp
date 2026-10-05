#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "esp_wifi.h"
#include <math.h>

#define TCS 33
#define TIRQ 36
#define TCLK 25
#define TMOSI 32
#define TMISO 39

TFT_eSPI tft;
SPIClass ts(VSPI);
XPT2046_Touchscreen touch(TCS,TIRQ);

struct Net{String ssid; int rssi; uint8_t enc;};
Net nets[10]; int nnet=0, selected=-1;
String pass="";
enum Page{HOME,SCAN,PASS,CONNECT,SENSE}; Page page=HOME;

volatile float csi=0; volatile int clen=0; volatile uint32_t packets=0;
volatile bool newCSI=false; float level=0,base=0; bool moving=false,sensing=false;

void cb(void*,wifi_csi_info_t*i){
  if(!i||!i->buf||i->len<=0)return;
  float s=0; for(int k=0;k<i->len;k++)s+=fabsf((float)i->buf[k]);
  csi=s/i->len; clen=i->len; packets++; newCSI=true;
}
void btn(int x,int y,int w,int h,const char*s){
  tft.drawRoundRect(x,y,w,h,5,TFT_WHITE); tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE); tft.setCursor(x+(w-tft.textWidth(s))/2,y+h/2-4); tft.print(s);
}
void head(const char*s){
  tft.fillScreen(TFT_BLACK); tft.fillRect(0,0,320,34,TFT_BLUE);
  tft.setTextColor(TFT_WHITE); tft.setTextSize(2); tft.setCursor(7,8); tft.print(s);
}
void home(){page=HOME;tft.fillScreen(TFT_BLACK);tft.setTextColor(TFT_CYAN);tft.setTextSize(2);tft.setCursor(48,70);tft.print("WIFI CSI");tft.setCursor(48,98);tft.print("SENSING");btn(65,155,190,50,"SCAN WIFI");}
void scan(){
 page=SCAN; head("WIFI SCAN"); tft.setTextSize(1);tft.setCursor(8,43);tft.print("Scanning 2.4 GHz...");
 WiFi.mode(WIFI_STA); WiFi.disconnect(true,true); delay(250);
 int n=WiFi.scanNetworks(false,true); nnet=0;
 for(int i=0;i<n&&nnet<10;i++){nets[nnet]={WiFi.SSID(i),WiFi.RSSI(i),(uint8_t)WiFi.encryptionType(i)};nnet++;}
 tft.fillRect(0,55,320,165,TFT_BLACK);
 for(int i=0;i<nnet;i++){int y=55+i*16;tft.drawRect(3,y,314,15,TFT_DARKGREY);tft.setCursor(7,y+4);String s=nets[i].ssid;if(s.length()>23)s=s.substring(0,23);tft.print(s);tft.setCursor(270,y+4);tft.print(nets[i].rssi);}
 btn(15,222,135,32,"RESCAN");btn(170,222,135,32,"HOME");WiFi.scanDelete();
}
void keyboard(){
 page=PASS;head("PASSWORD");tft.setTextSize(1);tft.setCursor(8,45);tft.print("Network: ");tft.print(nets[selected].ssid);
 tft.drawRect(7,60,306,28,TFT_WHITE);tft.setCursor(13,70);for(size_t i=0;i<pass.length();i++)tft.print('*');
 const char*k="1234567890QWERTYUIOPASDFGHJKL_ZXCVBNM.<O";
 for(int i=0;i<40;i++){int x=5+(i%10)*31,y=96+(i/10)*31;tft.drawRect(x,y,29,28,TFT_DARKGREY);tft.setCursor(x+10,y+10);tft.print(k[i]);}
 btn(5,225,95,30,"CANCEL");btn(110,225,100,30,"SPACE");btn(220,225,95,30,"CONNECT");
}
bool startCSI(){
 wifi_csi_config_t c={};c.lltf_en=true;c.htltf_en=true;c.stbc_htltf2_en=true;c.ltf_merge_en=true;
 if(esp_wifi_set_csi_config(&c)!=ESP_OK)return false;
 if(esp_wifi_set_csi_rx_cb(cb,nullptr)!=ESP_OK)return false;
 if(esp_wifi_set_csi(true)!=ESP_OK)return false;
 sensing=true;packets=0;level=base=0;page=SENSE;return true;
}
void connectNet(){
 page=CONNECT;head("CONNECTING");tft.setTextSize(1);tft.setCursor(10,55);tft.print(nets[selected].ssid);
 WiFi.mode(WIFI_STA);WiFi.begin(nets[selected].ssid.c_str(),pass.c_str());
 for(int i=0;i<30&&WiFi.status()!=WL_CONNECTED;i++)delay(500);
 if(WiFi.status()!=WL_CONNECTED){tft.setTextColor(TFT_RED);tft.setCursor(10,100);tft.print("Connection failed");btn(90,190,140,35,"BACK");return;}
 head("CONNECTED");tft.setTextColor(TFT_GREEN);tft.setTextSize(2);tft.setCursor(10,55);tft.print("OK");
 tft.setTextSize(1);tft.setTextColor(TFT_WHITE);tft.setCursor(10,90);tft.print("SSID: ");tft.print(nets[selected].ssid);
 tft.setCursor(10,110);tft.print("RSSI: ");tft.print(WiFi.RSSI());tft.print(" dBm");
 tft.setCursor(10,130);tft.print("IP: ");tft.print(WiFi.localIP());delay(900);
 if(!startCSI()){tft.fillScreen(TFT_BLACK);tft.setTextColor(TFT_RED);tft.setCursor(20,90);tft.print("CSI ERROR");delay(1200);home();}
}
void senseDraw(){
 static int x=7,oldy=108;
 tft.fillScreen(TFT_BLACK);tft.fillRect(0,0,320,34,TFT_BLUE);tft.setTextColor(TFT_WHITE);tft.setTextSize(2);tft.setCursor(7,8);tft.print("CSI SENSING");
 tft.setTextSize(1);tft.setCursor(7,43);tft.print("RSSI: ");tft.print(WiFi.RSSI());tft.print(" dBm");tft.setCursor(160,43);tft.print("Packets: ");tft.print(packets);
 tft.drawRect(5,55,310,105,TFT_WHITE);
 float v=level;if(v<0)v=0;if(v>100)v=100;int y=158-(int)(v*.98f);
 if(x>=313){tft.fillRect(7,57,306,101,TFT_BLACK);x=7;}
 if(x>7)tft.drawLine(x-1,oldy,x,y,TFT_GREEN);oldy=y;x+=2;
 tft.setTextSize(2);tft.setCursor(8,175);tft.setTextColor(TFT_WHITE);tft.print("Motion:");tft.setTextColor(moving?TFT_RED:TFT_GREEN);tft.print(moving?" YES":" NO");
 tft.setTextColor(TFT_WHITE);tft.setTextSize(1);tft.setCursor(8,202);tft.print("CSI length: ");tft.print(clen);tft.setCursor(8,218);tft.print("Level: ");tft.print(level,2);btn(235,205,75,30,"STOP");
}
void touchLoop(){
 if(!touch.touched())return;TS_Point p=touch.getPoint();int x=map(p.x,200,3800,0,320),y=map(p.y,200,3800,0,240);if(x<0||x>320||y<0||y>240)return;
 if(page==HOME){if(y>140)scan();}
 else if(page==SCAN){if(y>=55&&y<215){int i=(y-55)/16;if(i>=0&&i<nnet){selected=i;pass="";if(nets[i].enc==WIFI_AUTH_OPEN)connectNet();else keyboard();}}else if(y>=220&&x<155)scan();else if(y>=220)home();}
 else if(page==PASS){if(y>=96&&y<220){int i=(y-96)/31*10+x/31;if(i>=0&&i<40){if(i==38){if(pass.length())pass.remove(pass.length()-1);}else if(i==39){connectNet();return;}else{const char*k="1234567890QWERTYUIOPASDFGHJKL_ZXCVBNM.";pass+=k[i];}keyboard();}}else if(y>=220&&x<100)scan();else if(y>=220&&x>=220)connectNet();else if(y>=220&&x>=110) {pass+=' ';keyboard();}}
 else if(page==CONNECT){if(y>175)home();}
 else if(page==SENSE){if(y>190&&x>215){esp_wifi_set_csi(false);sensing=false;home();}}
}
void setup(){
 Serial.begin(115200);delay(300);tft.init();tft.setRotation(1);ts.begin(TCLK,TMISO,TMOSI,TCS);touch.begin(ts);touch.setRotation(1);WiFi.mode(WIFI_STA);home();
}
void loop(){
 touchLoop();
 if(sensing&&newCSI){newCSI=false;if(level==0)level=csi;level=.85f*level+.15f*csi;if(base==0)base=level;base=.995f*base+.005f*level;moving=fabsf(level-base)>8;}
 static uint32_t last=0;if(page==SENSE&&millis()-last>250){senseDraw();last=millis();}delay(10);
}