// Based on the library examples
// License: GPLv2

#include <EtherCard.h>

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

static byte ipserverping[] = { 192,168,0,100 };

static byte targetmac[] = {0x00,0x1C,0xC0,0x93,0x86,0xDC};

byte Ethernet::buffer[700];
static uint32_t timer;
static uint32_t timer2;
static uint32_t timer3;

boolean WOLsent = false;
boolean primerPing = false;

const char website[] PROGMEM = "raw.githack.com";

// called when the client request is complete
static void my_callback (byte status, word off, word len) {
  Ethernet::buffer[off+len] = 0;
  const char* data = (const char*) Ethernet::buffer + off;
  Serial.println(">>>");
  Serial.println(data[9]);
  Serial.print(data);
  Serial.println("...");
  // Si se esta el archivo entonces enviamos el paquete.
  if (data[9]==0x32 && !WOLsent){
    WOLsent = true;
    Serial.println("Magic packet sent!");
    ether.sendWol(targetmac);
  } else {
    WOLsent = false;
  }
}

// called when a ping comes in (replies to it are automatic)
static void gotPinged (byte* ptr) {
  ether.printIp(">>> ping from: ", ptr);
}

void setup () {
  Serial.begin(57600);
  Serial.println(F("\n[webClient]"));

  // Change 'SS' to your Slave Select pin, if you arn't using the default pin
  if (ether.begin(sizeof Ethernet::buffer, mymac, SS) == 0)
    Serial.println(F("Failed to access Ethernet controller"));
  if (!ether.dhcpSetup())
    Serial.println(F("DHCP failed"));

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);
  ether.printIp("DNS: ", ether.dnsip);

#if 1
  // use DNS to resolve the website's IP address
  if (!ether.dnsLookup(website))
    Serial.println("DNS failed");
#elif 2
  // if website is a string containing an IP address instead of a domain name,
  // then use it directly. Note: the string can not be in PROGMEM.
  char websiteIP[] = "192.168.1.1";
  ether.parseIp(ether.hisip, websiteIP);
#else
  // or provide a numeric IP address instead of a string
  byte hisip[] = { 192,168,1,1 };
  ether.copyIp(ether.hisip, hisip);
#endif

// call this to report others pinging us
  ether.registerPingCallback(gotPinged);

  ether.printIp("SRV: ", ether.hisip);
}

void loop () {
  word len = ether.packetReceive(); // go receive new packets
  word pos = ether.packetLoop(len); // respond to incoming pings

  if (!primerPing) {
      ether.printIp("Primer Ping: ", ipserverping);
      ether.clientIcmpRequest(ipserverping);
      timer2 = micros();
      timer3 = millis() + 5000;
      primerPing = true;
  } else {
    // ping a remote server once every few seconds
    if (micros() - timer2 >= 5000000) {
      ether.printIp("Pinging: ", ipserverping);
      timer2 = micros();
      ether.clientIcmpRequest(ipserverping);
      timer3 = millis() + 5000;
    }
  
    // report whenever a reply to our outgoing ping comes back
    if (len > 0 && ether.packetLoopIcmpCheckReply(ipserverping)) {
      Serial.print("Responde con ");
      Serial.print((micros() - timer2) * 0.001, 3);
      Serial.println(" ms");
      // Responde no intente enviar el paquete magico.
      WOLsent = false;
    } else {
      // No responde
      // Ya esperamos por el primer ping?
      if (millis() > timer && millis() > timer3) {
        timer = millis() + 30000;
        Serial.println();
        Serial.print("<<< REQ ");
        ether.browseUrl(PSTR("/gislersoft/iot/master/"), "config.json", website, my_callback);
      }
    }
  }

}