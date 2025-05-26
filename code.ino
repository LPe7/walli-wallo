#include "Servo.h"

#define PIN_SERVO_L 9
#define PIN_SERVO_R 8

#define SR_STOP 1470
#define SR_AVANT 1258
#define SR_AVANT_MOYEN 1370
#define SR_ARRIERE 1660
#define SR_ARRIERE_MOYEN 1570

#define SL_STOP 1470
#define SL_AVANT 1790
#define SL_AVANT_MOYEN 1575
#define SL_ARRIERE 1190
#define SL_ARRIERE_MOYEN 1340

#define BBL A1
#define BFL A0
#define BBR A4
#define BFR A5

#define CNY_SEUIL 800
#define CNY_READ(which) (analogRead(which) > CNY_SEUIL)

#define CNY_FL_FL 0b0001
#define CNY_FL_FR 0b0010
#define CNY_FL_BL 0b0100
#define CNY_FL_BR 0b1000

Servo servo_r, servo_l;

enum Direction {
  AVANT,
  AVANT_MOYEN,
  AVANT_GAUCHE,
  AVANT_DROITE,
 
  ARRIERE,
  ARRIERE_MOYEN,
  ARRIERE_GAUCHE,
  ARRIERE_DROITE,
 
  GAUCHE,
  DROITE,
  GAUCHE_LARGE,
  DROITE_LARGE,

  STOP,
};

enum CnyState {
  CNY_ST_FL,
  CNY_ST_FR,
  CNY_ST_FL_R,
  CNY_ST_FR_L,
 
  CNY_ST_BL,
  CNY_ST_BR,
  CNY_ST_BL_R,
  CNY_ST_BR_L,

  CNY_ST_LINE_L,
  CNY_ST_LINE_R,

  CNY_NONE,
};

CnyState cny_state = CNY_NONE;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(9600);

  // attendre qu'une feuille blanche soit glissée sous le CNY de derrière droite
  while (!CNY_READ(BBR));
  //while(analogRead(BBR) < CNY_SEUIL);

  for (byte i = 0; i<3; i++) {
	digitalWrite(LED_BUILTIN, HIGH);
	delay(500);
	digitalWrite(LED_BUILTIN, LOW);
	delay(500);
  }
  digitalWrite(LED_BUILTIN, HIGH);

  servo_r.attach(PIN_SERVO_R);
  servo_r.writeMicroseconds(SR_STOP);
  servo_l.attach(PIN_SERVO_L);
  servo_l.writeMicroseconds(SL_STOP);

  servo_l.writeMicroseconds(SL_STOP);
  servo_r.writeMicroseconds(SR_STOP);
}

void loop() {
  byte cny_flags = 0;
  if (CNY_READ(BFL)) cny_flags |= CNY_FL_FL;
  if (CNY_READ(BFR)) cny_flags |= CNY_FL_FR;
  if (CNY_READ(BBL)) cny_flags |= CNY_FL_BL;
  if (CNY_READ(BBR)) cny_flags |= CNY_FL_BR;

  Serial.println(cny_flags);

  if (cny_flags != 0) {
	switch(cny_flags) {
  	case CNY_FL_FL:
    	cny_state = CNY_ST_FL;
    	move(DROITE);
    	break;
   
  	case CNY_FL_FR:
    	move(GAUCHE);
    	cny_state = CNY_ST_FR;
    	break;
   
  	case CNY_FL_BL:
    	move(AVANT_DROITE);
    	cny_state = CNY_ST_BL;
    	break;
   
  	case CNY_FL_BR:
    	move(AVANT_GAUCHE);
    	cny_state = CNY_ST_BR;
    	break;
   
  	case CNY_FL_FL | CNY_FL_FR:
    	switch (cny_state) {
      	case CNY_ST_FL:
        	cny_state = CNY_ST_FL_R,
        	move(DROITE);
        	break;

      	case CNY_NONE: // par défaut : on tourne à gauche
      	case CNY_ST_FR:
      	default:
        	cny_state = CNY_ST_FR_L,
        	move(GAUCHE);
        	break;
    	}
    	break;
   
  	case CNY_FL_BL | CNY_FL_BR:
    	switch (cny_state) {
      	case CNY_ST_BL:
        	cny_state = CNY_ST_BL_R,
        	move(AVANT_DROITE);
        	break;

      	case CNY_NONE: // par défaut : on tourne à droite
      	case CNY_ST_BR:
      	default:
        	cny_state = CNY_ST_BR_L,
        	move(AVANT_GAUCHE);
        	break;
    	}
    	break;
	}
  } else {
	cny_state = CNY_NONE;
	move(AVANT);
  }
}

void move(Direction direction) {
  switch (direction) {
	case AVANT:
  	servo_r.writeMicroseconds(SR_AVANT);
  	servo_l.writeMicroseconds(SL_AVANT);
  	break;
	case AVANT_MOYEN:
  	servo_r.writeMicroseconds(SR_AVANT_MOYEN);
  	servo_l.writeMicroseconds(SL_AVANT_MOYEN);
  	break;
	case AVANT_GAUCHE:
  	servo_r.writeMicroseconds(SR_AVANT);
  	servo_l.writeMicroseconds(SL_AVANT_MOYEN);
  	break;
	case AVANT_DROITE:
  	servo_r.writeMicroseconds(SR_AVANT_MOYEN);
  	servo_l.writeMicroseconds(SL_AVANT);
  	break;
 
	case ARRIERE:
  	servo_r.writeMicroseconds(SR_ARRIERE);
  	servo_l.writeMicroseconds(SL_ARRIERE);
  	break;
	case ARRIERE_MOYEN:
  	servo_r.writeMicroseconds(SR_ARRIERE_MOYEN);
  	servo_l.writeMicroseconds(SL_ARRIERE_MOYEN);
  	break;
	case ARRIERE_GAUCHE:
  	servo_r.writeMicroseconds(SR_ARRIERE);
  	servo_l.writeMicroseconds(SL_ARRIERE_MOYEN);
  	break;
	case ARRIERE_DROITE:
  	servo_r.writeMicroseconds(SR_ARRIERE_MOYEN);
  	servo_l.writeMicroseconds(SL_ARRIERE);
  	break;
 
	case GAUCHE:
  	servo_r.writeMicroseconds(SR_AVANT);
  	servo_l.writeMicroseconds(SL_ARRIERE);
  	break;
	case DROITE:
  	servo_r.writeMicroseconds(SR_ARRIERE);
  	servo_l.writeMicroseconds(SL_AVANT);
  	break;
	case GAUCHE_LARGE:
  	servo_r.writeMicroseconds(SR_AVANT);
  	servo_l.writeMicroseconds(SL_STOP);
  	break;
	case DROITE_LARGE:
  	servo_r.writeMicroseconds(SR_STOP);
  	servo_l.writeMicroseconds(SL_AVANT);
  	break;
 
	case STOP:
  	servo_r.writeMicroseconds(SR_STOP);
  	servo_l.writeMicroseconds(SL_STOP);
  	break;
  }
}
