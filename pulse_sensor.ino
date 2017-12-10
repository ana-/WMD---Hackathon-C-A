
//  Variables
int pulsePin = 0;                 // Pulse Sensor purple wire connected to analog pin 0
int blinkPin = 13;                // pin to blink led at each beat
int fadePin = 8;                  // pin to do fancy classy fading blink at each beat
int fadeRate = 0;                 // used to fade LED on with PWM on fadePin

// Volatile Variables, used in the interrupt service routine!
volatile int BPM;                   // int that holds raw Analog in 0. updated every 2mS
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // int that holds the time interval between beats! Must be seeded! 
volatile boolean Pulse = false;     // "True" when User's live heartbeat is detected. "False" when not a "live beat". 
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.

// Regards Serial OutPut  -- Set This Up to your needs
static boolean serialVisual = true;   // Set to 'false' by Default.  Re-set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse 

volatile int rate[10];                      // array to hold last ten IBI values
volatile unsigned long sampleCounter = 0;          // used to determine pulse timing
volatile unsigned long lastBeatTime = 0;           // used to find IBI
volatile int P = 512;                      // used to find peak in pulse wave, seeded
volatile int T = 512;                     // used to find trough in pulse wave, seeded
volatile int thresh = 525;                // used to find instant moment of heart beat, seeded
volatile int amp = 100;                   // used to hold amplitude of pulse waveform, seeded
volatile boolean firstBeat = true;        // used to seed rate array so we startup with reasonable BPM
volatile boolean secondBeat = false;      // used to seed rate array so we startup with reasonable BPM

void setup()
{
  pinMode(blinkPin,OUTPUT);         
  pinMode(fadePin,OUTPUT);          
  Serial.begin(115200);          
  interruptSetup();                 
                                       
}


void loop()
{
   serialOutput();  
   
  if (QS == true) // A Heartbeat Was Found
    {     
      
      fadeRate = 255; 
      serialOutputWhenBeatHappens(); 
      QS = false; 
    }
     
  ledFadeToBeat();
  delay(20);
}

void ledFadeToBeat()
{
   fadeRate -= 15;                         
   fadeRate = constrain(fadeRate,0,255);   
   analogWrite(fadePin,fadeRate);          
}

void interruptSetup()
{     
  TCCR2A = 0x02;     
  TCCR2B = 0x06;      
  OCR2A = 0X7C;    
  TIMSK2 = 0x02;     
  sei();                  
} 

void serialOutput()
{   // Decide How To Output Serial. 
 if (serialVisual == true)
  {  
     arduinoSerialMonitorVisual('-', Signal);   
  } 
 else
  {
      sendDataToSerial('S', Signal);     
   }        
}

void serialOutputWhenBeatHappens()
{    
 if (serialVisual == true) 
   {            
     Serial.print("*** Heart-Beat Happened *** "); 
     Serial.print("BPM: ");
     Serial.println(BPM);
   }
 else
   {
     sendDataToSerial('B',BPM);   
     sendDataToSerial('Q',IBI);   
   }   
}

void arduinoSerialMonitorVisual(char symbol, int data )
{    
  const int sensorMin = 0;      
  const int sensorMax = 1024;    
  int sensorReading = data; 
  int range = map(sensorReading, sensorMin, sensorMax, 0, 11);
  
  switch (range) 
  {
    case 0:     
      Serial.println("");     
      break;
    case 1:   
      Serial.println("---");
      break;
    case 2:    
      Serial.println("------");
      break;
    case 3:    
      Serial.println("---------");
      break;
    case 4:   
      Serial.println("------------");
      break;
    case 5:   
      Serial.println("--------------|-");
      break;
    case 6:   
      Serial.println("--------------|---");
      break;
    case 7:   
      Serial.println("--------------|-------");
      break;
    case 8:  
      Serial.println("--------------|----------");
      break;
    case 9:    
      Serial.println("--------------|----------------");
      break;
    case 10:   
      Serial.println("--------------|-------------------");
      break;
    case 11:   
      Serial.println("--------------|-----------------------");
      break;
  } 
}


void sendDataToSerial(char symbol, int data )
{
   Serial.print(symbol);
   Serial.println(data);                
}

ISR(TIMER2_COMPA_vect) 
{  
  cli();                                     
  Signal = analogRead(pulsePin);              
  sampleCounter += 2;                        
  int N = sampleCounter - lastBeatTime;       
                                              
  if(Signal < thresh && N > (IBI/5)*3) 
    {      
      if (Signal < T)
      {                        
        T = Signal;
      }
    }

  if(Signal > thresh && Signal > P)
    {          
      P = Signal;                             
    }                                        
  
  if (N > 250)
  {                                  
    if ( (Signal > thresh) && (Pulse == false) && (N > (IBI/5)*3) )
      {        
        Pulse = true;                              
        digitalWrite(blinkPin,HIGH);               
        IBI = sampleCounter - lastBeatTime;         
        lastBeatTime = sampleCounter;               // keep track of time for next pulse
  
        if(secondBeat)
        {                       
          secondBeat = false;                
          for(int i=0; i<=9; i++) 
          {             
            rate[i] = IBI;                      
          }
        }
  
        if(firstBeat)
        {                         
          firstBeat = false;                 
          secondBeat = true;                   
          sei();                               
          return;                            
        }   
      
      word runningTotal = 0;                   

      for(int i=0; i<=8; i++)
        {               
          rate[i] = rate[i+1];                 
          runningTotal += rate[i];            
        }

      rate[9] = IBI;                        
      runningTotal += rate[9];                
      runningTotal /= 10;                     
      BPM = 60000/runningTotal;               
      QS = true;                              
      // QS FLAG IS NOT CLEARED INSIDE THIS ISR
    }                       
  }

  if (Signal < thresh && Pulse == true)
    {  
      digitalWrite(blinkPin,LOW);           
      Pulse = false;                        
      amp = P - T;                           
      thresh = amp/2 + T;                    
      P = thresh;                            
      T = thresh;
    }

  if (N > 2500)
    {                          
      thresh = 512;                         
      P = 512;                              
      T = 512;                               
      lastBeatTime = sampleCounter;                 
      firstBeat = true;                      
      secondBeat = false;                    
    }

  sei();                                 
}





