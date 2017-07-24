/*
ClimoBike - Sistema de monitoramento automatizado georreferenciado
*/

#include <SHT2x.h>    //header file for SHT2x functions
#include <I2C_HAL.h> //header file for I2C hardware abstraction
#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <TinyGPS++.h>

#define ECHO_TO_SERIAL 1         //Envia registro de dados para serial se 1, nada se 0
#define LOG_INTERVAL 10000     //Intervado de medidas (6 minutes = 360000)

#define UVPin 0            //output sensor UV
#define shtPin 2           // output sensor SHT
#define chipSelect 10      //output da placa sd (infos medidas pelos sensores)
#define LEDPinVerde 6      //output LED verde
#define LEDPinVermelho 7   //output LED vermelha

RTC_DS1307 rtc;

float Umidade = 0;           //umidade relativa (%)
float TempAr = 0;            //temperatura do ar (degrees F)
float UV = 0;          //radiacao UV
DateTime now;
File logfile;


void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  
  // LED vermelho indicando erro
  digitalWrite(LEDPinVermelho, HIGH);
  
  while(1);
}

void setup() {
  
  //Iniciar conexao serial
  Serial.begin(9600); //apenas para teste
  Serial.println("Iniciando SD...");
  
  
  pinMode(chipSelect, OUTPUT); //Pin para gravar no cartao SD
  pinMode(LEDPinVerde, OUTPUT); //pin da LED verde
  pinMode(LEDPinVermelho, OUTPUT); //pin da LED vermelha
  analogReference(EXTERNAL); //Define a tensão máxima das entradas analógicas para o que está conectado a saida Aref (deve ser de 3.3v)
  
  //Estabelece conexao com o sensor SHT
  sht.init();
  
  //estabelece conexao com o relogio em tempo real
  Wire.begin();
  if (!rtc.begin()) {
    logfile.println("RTC falhou");
#if ECHO_TO_SERIAL
    Serial.println("RTC falhou");
#endif  //ECHO_TO_SERIAL
  }
  
  //Define data e hora em tempo real se necessario
  if (! rtc.isrunning()) {
    // Linhas seguintes define a data e a hora do RTC no momento que for compilado
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  //teste se o SD esta presente e inicia
  if (!SD.begin(chipSelect)) {
    error("SD com defeito ou ausente");        
  }
  
  Serial.println("SD iniciado");
  
  // criar novo arquivo
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // apenas abre um novo arquivo se ele nao existe
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // abandonar o loop!
    }
  }
  
  if (! logfile) {
    error("impossvel criar arquivo");
  }
  
  Serial.print("Logging to: ");
  Serial.println(filename);
  
  
  logfile.println("GPS, Unix Time (s),Date,Air Temp (C), Air Humidity,UV data");   //HEADER 
#if ECHO_TO_SERIAL
  Serial.println("GPS, Unix Time (s),Date,Air Temp (C), Air Humidity,UV data");
#endif ECHO_TO_SERIAL// attempt to write out the header to the file

  now = rtc.now();
    
}

void loop() {
    
  //delay software
  delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));
  
  //tres piscadas significa inicio de um novo ciclo
  digitalWrite(LEDPinVerde, HIGH);
  delay(150);
  digitalWrite(LEDPinVerde, LOW);
  delay(150);
  digitalWrite(LEDPinVerde, HIGH);
  delay(150);
  digitalWrite(LEDPinVerde, LOW);
  delay(150);
  digitalWrite(LEDPinVerde, HIGH);
  delay(150);
  digitalWrite(LEDPinVerde, LOW);
  
  now = rtc.now();
  
  // log time
  logfile.print(now.unixtime()); // segundos desde 2000
  logfile.print(",");
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
  logfile.print(",");
 #if ECHO_TO_SERIAL
  Serial.print(now.unixtime()); // segundos desde 2000
  Serial.print(",");
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
  Serial.print(",");
#endif //ECHO_TO_SERIAL
  
  Umidade = sht.getHumidity();
  delay(20);
  
  Temperatura = sht.getTemperature(true);
  delay(20);
    
  //Log variables
  logfile.print(Temperatura);
  logfile.print(",");
  logfile.print(Umidade);
  logfile.print(",");
  logfile.print(UV);
  logfile.print(",");
#if ECHO_TO_SERIAL
  Serial.print(Temperatura);
  Serial.print(",");
  Serial.print(Umidade);
  Serial.print(",");
  Serial.print(UV);
  Serial.print(",");
#endif
  
  delay(50);
  
  //gravar dados no SD
  logfile.flush();
  delay(5000);
}
