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

#define TR_TRG 1
#define TR_ECH 0
#define TC_TRG 2
#define TC_ECH 3
#define TL_TRG 5
#define TL_ECH 4

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

#define CNY_POST_DELAY 1000

Servo servo_r, servo_l;

enum Direction {
  AVANT,
  AVANT_MOYEN,
  AVANT_GAUCHE,
  AVANT_GAUCHE_FORT,
  AVANT_DROITE,
  AVANT_DROITE_FORT,
 
  ARRIERE,
  ARRIERE_MOYEN,
  ARRIERE_GAUCHE,
  ARRIERE_GAUCHE_FORT,
  ARRIERE_DROITE,
  ARRIERE_DROITE_FORT,
 
  GAUCHE,
  DROITE,

  STOP,
};

enum Vision {
    VISION_DROIT,
    VISION_GAUCHE,
    VISION_CENTRE,
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

  CNY_ST_LALL,
  CNY_ST_RALL,

	CNY_POST_STL,
	CNY_POST_STR,
  CNY_NONE,
};

CnyState cny_state = CNY_NONE;
unsigned long cny_state_timer = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

	pinMode(TR_TRG, OUTPUT);
	pinMode(TR_ECH, INPUT);
	pinMode(TC_TRG, OUTPUT);
	pinMode(TC_ECH, INPUT);
	pinMode(TL_TRG, OUTPUT);
	pinMode(TL_ECH, INPUT);

  // attendre qu'une feuille blanche soit glissée sous le CNY de derrière droite
  while (!CNY_READ(BBR));

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

  move(ARRIERE);
	delay(200);
	move(STOP);
	delay(100);
	move(AVANT);
	delay(200);
}

void loop() {
  byte cny_flags = 0;
  if (CNY_READ(BFL)) cny_flags |= CNY_FL_FL;
  if (CNY_READ(BFR)) cny_flags |= CNY_FL_FR;
  if (CNY_READ(BBL)) cny_flags |= CNY_FL_BL;
  if (CNY_READ(BBR)) cny_flags |= CNY_FL_BR;

  //Serial.println(cny_flags);

  if (cny_flags != 0) {
		switch(cny_flags) {

			// 1 capteur
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

			// 2 capteurs
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

			case CNY_FL_FL | CNY_FL_BL:
				move(AVANT_DROITE);
				cny_state = CNY_ST_LALL;
				break;

			case CNY_FL_FR | CNY_FL_BR:
				move(AVANT_GAUCHE);
				cny_state = CNY_ST_RALL;
				break;

			case CNY_FL_BL | CNY_FL_BR:
				switch (cny_state) {
					case CNY_ST_BL:
						cny_state = CNY_ST_BL_R;
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
			
			// 3 ou 4 capteurs:
			default:
				///////////////////////////////////////////////////////////////////////////////// TODO
				break;
		}
  } else {
		if (cny_state != CNY_NONE) {
			switch (cny_state) {
				// une fois le recentrage terminé
				case CNY_POST_STL:
				case CNY_POST_STR:
					if (millis() - cny_state_timer > CNY_POST_DELAY) {
						cny_state = CNY_NONE;
					}
					break;

				// on revient vers le centre à droite
				case CNY_ST_FL:
				case CNY_ST_BL:
				case CNY_ST_LALL:
					cny_state = CNY_POST_STL;
					cny_state_timer = millis();
					move(AVANT_DROITE_FORT);
					break;
				
				// par défaut
				// on revient vers le centre à gauche
				case CNY_ST_FR:
				case CNY_ST_BR:
				case CNY_ST_RALL:
					cny_state = CNY_POST_STR;
					cny_state_timer = millis();
					move(AVANT_GAUCHE_FORT);
					break;
			}
		} else {
			move(AVANT_MOYEN);
		}
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
		case AVANT_GAUCHE_FORT:
			servo_r.writeMicroseconds(SR_AVANT);
			servo_l.writeMicroseconds(SL_STOP);
			break;
		case AVANT_DROITE:
			servo_r.writeMicroseconds(SR_AVANT_MOYEN);
			servo_l.writeMicroseconds(SL_AVANT);
			break;
		case AVANT_DROITE_FORT:
			servo_r.writeMicroseconds(SR_STOP);
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
		case ARRIERE_GAUCHE_FORT:
			servo_r.writeMicroseconds(SR_ARRIERE);
			servo_l.writeMicroseconds(SL_STOP);
			break;
		case ARRIERE_DROITE:
			servo_r.writeMicroseconds(SR_ARRIERE_MOYEN);
			servo_l.writeMicroseconds(SL_ARRIERE);
			break;
		case ARRIERE_DROITE_FORT:
			servo_r.writeMicroseconds(SR_STOP);
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
	
		case STOP:
			servo_r.writeMicroseconds(SR_STOP);
			servo_l.writeMicroseconds(SL_STOP);
			break;
  }
}

float distance(Vision vision) {
	float duration_us, distance_cm;
	int pin_echo, pin_trig;
  
	switch (vision) {
    case VISION_GAUCHE:
      pin_echo = TL_ECH;
      pin_trig = TL_TRG;
     break;
    case VISION_CENTRE:
      pin_echo = TC_ECH;
      pin_trig = TC_TRG;
     break;
    case DROITE:
      pin_echo = TR_ECH;
      pin_trig = TR_TRG;
     break;
	}
	
	digitalWrite(pin_trig, HIGH);
	delayMicroseconds(10);
	digitalWrite(pin_trig, LOW);

	duration_us = pulseIn(pin_echo, HIGH, 10000);
	distance_cm = 0.017 * duration_us;
	
	return(distance_cm);
}