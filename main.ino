int pwd[4] = {10, 11, 12, 13}; //preset password (blue, yellow, green, red)
int pwdDefault[4] = {10, 11, 12, 13};
int pwdAttempt[4] = {0};

bool entry = 0;
bool resetMode = 0;
bool setPwd = 0;

int flash = 300;
bool flashing = 0; //Tracks whether program is in midst of flashing

volatile int activePin = 0;
const int reset = 9;
const int blue = 10;
const int yellow = 11;
const int green = 12;
const int red = 13;

unsigned long last_interrupt_time = 0;
unsigned long interrupt_time = 0;

volatile unsigned long firstPush = 0;
int timeoutValue = 10000;  //Give 10s to enter password
bool timeout = 0;
bool timeoutDisable = 0;

bool bounce = 0;
bool interruptLock = 0;   //If true will ignore interrupt

bool buttonState = 0; //Tracks whether button is high (pushed) or low (released)

unsigned long overflows = 0;




void setup(){
  cli();
  Serial.begin(9600);
  // put your setup code here, to run once:
  cli();
  DDRB = 1;  //set pin 8 to write and 9-13 to read
  DDRD |= 0b11111000; //set pins 0-2 to read, 3-7 to write
  
  TIMSK1 |= 1;

  //Set prescaler to clk/64
  TCCR1B &= ~(1<<2);
  TCCR1B |= (1<<1);
  TCCR1B |= (1<<0);
  
  attachInterrupt(0,buttonPushed,RISING);
  sei();
}




void loop() {
  
  if (entry){

    int j = 1;  //Variable that sets number of loops (4 for reset, 3 for attempt)

    if (activePin == reset){
      Serial.println("resetMode active");
      j = 0;
      resetMode = 1;
      clearPwd(pwdAttempt);
      timeoutDisable = 1;
    }

    if(setPwd){
      Serial.println("Pwd reset mode engaged");
    }
    
    if (!resetMode)
      digitalWrite(6, HIGH);


    for (int i = j; i < 4; i++){
      Serial.println("\nTop of loop");
      Serial.print("ActivePin: ");
      Serial.println(activePin);
      
      //Determine if button is being pressed and is not just noise
      if (digitalRead(2)==HIGH && !bounce)
        buttonState = 1;


      if(buttonState = 1)
        interruptLock = 1;
        
      //Wait for user to release button if high
      while(buttonState == 1){
        if(digitalRead(2) == LOW){
          //last_down_time = myMillis();
          buttonState = 0;
        }
        
      }

      //Wait for high to low noise to dissipate
      myDelay(100);

      interruptLock = 0;

      
      //State of button: low
      
      
      while(digitalRead(2) == LOW){ //Wait until next button is pressed
        //Check if timeout:
        if (!resetMode || i > 0)
          if (checkTimeout(firstPush, timeoutValue)){
            Serial.println("timeout detected with button low");
            break;
          }
      }

      if (resetMode && i == 0){
        firstPush = myMillis();
        Serial.println("First push set for reset");
      }
        
      //Check if timeout:
      if (checkTimeout(firstPush, timeoutValue)){//&&!setPwd;
        Serial.println("Timeout! Password cleared");
        flashRedBlue(i);
        clearPwd(pwdAttempt); 
        entry = 0;
        timeout = 1;
        timeoutDisable = 1;
        break;
      }


      digitalWrite(6-i, HIGH);
      
      
      Serial.print("Button for password element ");
      Serial.print(i);
      Serial.println(" pressed.");

      //State of button: high
      //At this point a button should be pressed for the second, third or fourth time
      
      //if (!bounce){
        Serial.print("Entering ");
        Serial.print(activePin);
        Serial.println(" to input sequence");

        if (!setPwd)
          pwdAttempt[i] = activePin;

        else
          pwd[i] = activePin;
      //}
        
        Serial.print("i: ");
        Serial.println(i);
        
        if (i == 3){
          Serial.println("Loop terminated, entry set to 0 unless resetMode active");

          //if(!resetMode)
            entry = 0;

          //if(setPwd)
          //  setPwd = 0;
        }
    
    }

    if (timeout)
      entry = 0;
  
    if(!setPwd)
      printPwd(pwdAttempt);

    else{
      Serial.println("New password:");
      printPwd(pwd);
      flashGreen();
    }


    if (!timeout && !setPwd){  //Else red/green will flash after blue
      if (isCorrect(pwdAttempt))
        flashGreen();
      
      else{
        flashRed();
      }
    }

    setPwd = 0;

    if(resetMode && isCorrect(pwdAttempt)){
      setPwd = 1;
      resetMode = 0; //(ensures user is 'locked out' after entering new pwd)
    }
    
    clearPwd(pwdAttempt);
    timeout = 0;
   
  }



//  if (entry == 0 || resetMode){

    digitalWrite(6, LOW);
    digitalWrite(5, LOW);
    digitalWrite(4, LOW);
    digitalWrite(3, LOW);
    
//  }


  if (entry == 0)
    timeoutDisable = 1;
  
}




//Interrupt handler:

void buttonPushed(){

  //Catch timestamp at which interrupt is triggered
  interrupt_time = myMillis();
  
  // If interrupts come faster than 500ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 500 && !interruptLock){
    
    //Serial.println("\n----------------\nInterrupt triggered");
    
    //Determine which button is being pressed:
    
    if (digitalRead(blue) == HIGH)
      activePin = blue;

    else if (digitalRead(reset) == HIGH)
      activePin = reset;
    
    else if(digitalRead(yellow) == HIGH)
      activePin = yellow;
    
    else if (digitalRead(green) == HIGH)
      activePin = green;
    
    else if (digitalRead(red) == HIGH)
      activePin = red;
   
    bounce = 0;
  
    if (entry == 0){
      //Start password timer:
      timeoutDisable = 0;
      //Serial.println("Entry 0, firstPush reset");

      //Enter first element of password if not reset button
      if (activePin != reset && !setPwd)
        pwdAttempt[0] = activePin;

      else if (setPwd){
        pwd[0] = activePin;
      }
      
      firstPush = myMillis();
      entry = 1;
    }
    
    
  }

  else{  //If less than 200ms
    //Serial.println("Debounced!");
    bounce = 1;
  }

  last_interrupt_time = interrupt_time;
}




ISR(TIMER1_OVF_vect){
  overflows++;
}




unsigned long myMillis(){

  return ((overflows*65535 + TCNT1) / 32140);
  
}




void myDelay(int value){

  unsigned long startTime = myMillis();

  while (myMillis() - startTime < value){}

  return;
}




void flashGreen(){

  Serial.println("flashGreen() active");
  
  for(int i = 0; i < 10; i++){
    PORTB |= (1<<0);
    //TCCR1B |= (1<<2);
    //TCCR1B |= (1<<0);
    myDelay(flash);
    PORTB &= ~(1<<0);
    myDelay(flash);
  }
  
  return;
}




void flashRed(){

  Serial.println("flashRed() active");
  
  for (int i = 0; i < 10; i++){
    PORTD |= (1<<7);
    //TCCR1B |= (1<<2);
    //TCCR1B |= (1<<0);
    myDelay(flash);
    PORTD &= ~(1<<7);
    myDelay(flash);
  }

  return;
}




void flashRedBlue(int lightsActive){

  for (int i = 0; i < 6; i++){

    //Pins on:
    digitalWrite(7, HIGH);

    for (int i = 0; i < lightsActive; i++){
      int pin = 6 - i;
      digitalWrite(pin, HIGH);
    }
  
    myDelay(flash);
  
    
    //Pins off:
    digitalWrite(7, LOW);
  
    for (int i = 0; i < lightsActive; i++){
      int pin = 6 - i;
      digitalWrite(pin, LOW);
    }
  
    myDelay(flash);
  }

  return;
  
}




bool isCorrect(int pwdAttempt[]){
  
  bool correct = true;
  
  for ( int i = 0; i < 4; i++){
    if (pwdAttempt[i] != pwd[i])
      correct = false;
  }
  
  return correct;
}




void printPwd(int pwdAttempt[]){

  Serial.print("Password entered is: ");
  
  for (int i = 0; i < 4; i++){
    Serial.print(pwdAttempt[i]);
    Serial.print(" ");
  }

  Serial.println();
  
}



bool checkTimeout(int firstPush, int timeoutValue){

  bool timeoutFlag = 0;

  if ((myMillis() - firstPush > timeoutValue) && !timeoutDisable){
    timeoutFlag = 1;
  }

  if (timeoutDisable)
    Serial.println("timeoutDisable has denied a timeout");

  return timeoutFlag;
}


void clearPwd(int pwdAttempt[]){

  for (int i = 0; i < 4; i++){
    pwdAttempt[i] = 0;
  }

  return;
}


  

