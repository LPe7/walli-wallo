#include "Servo.h"

/// Définition des constantes

// constantes servomoteurs (Sx)
#define PIN_SERVO_L 9
#define PIN_SERVO_R 8

#define SR_STOP 1470
#define SR_AVANT 1258
#define SR_AVANT_LENT 1370
#define SR_ARRIERE 1660
#define SR_ARRIERE_LENT 1570

#define SL_STOP 1470
#define SL_AVANT 1790
#define SL_AVANT_LENT 1575
#define SL_ARRIERE 1190
#define SL_ARRIERE_LENT 1340

// constantes capteurs ultrasons (Tx)
#define PIN_TR_TRG 1
#define PIN_TR_ECH 0
#define PIN_TC_TRG 2
#define PIN_TC_ECH 3
#define PIN_TL_TRG 5
#define PIN_TL_ECH 4

// constantes capteurs infrarouges (Bxx et CNY_xxx)
#define PIN_BFL A0
#define PIN_BFR A5
#define PIN_BBL A1
#define PIN_BBR A4

#define FLAG_BFL 0b0001
#define FLAG_BFR 0b0010
#define FLAG_BBL 0b0100
#define FLAG_BBR 0b1000

#define CNY_POST_DELAY 1000
#define CNY_SEUIL 800
#define CNY_READ(which) (analogRead(which) > CNY_SEUIL) // macro pour lire la valeur d'un CNY

/// Déclaration des enums d'état et de commande

// mouvement
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

// capteurs ultrasons
enum Vision {
    VISION_DROIT,
    VISION_GAUCHE,
    VISION_CENTRE,
};

// capteurs infrarouges
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
  CNY_ST_NONE,
	CNY_ST_OUTSIDE,
};

/// Fonctions et variables principales

// variables générales
Servo servo_r, servo_l;
CnyState cny_state = CNY_ST_NONE;
unsigned long cny_state_timer = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

	pinMode(PIN_TR_TRG, OUTPUT);
	pinMode(PIN_TR_ECH, INPUT);
	pinMode(PIN_TC_TRG, OUTPUT);
	pinMode(PIN_TC_ECH, INPUT);
	pinMode(PIN_TL_TRG, OUTPUT);
	pinMode(PIN_TL_ECH, INPUT);

  // attendre qu'une feuille blanche soit glissée sous le CNY de derrière droite
  while (!CNY_READ(PIN_BBR));

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

	// faire tomber la plaque
  move(AVANT);
	delay(200);
	move(ARRIERE);
	delay(200);
	move(STOP);
	delay(100);
}

void loop() {
  byte cny_flags = 0;
  if (CNY_READ(PIN_BFL)) cny_flags |= FLAG_BFL;
  if (CNY_READ(PIN_BFR)) cny_flags |= FLAG_BFR;
  if (CNY_READ(PIN_BBL)) cny_flags |= FLAG_BBL;
  if (CNY_READ(PIN_BBR)) cny_flags |= FLAG_BBR;

  if (cny_flags != 0) {
		switch(cny_flags) {

			// 1 capteur
			case FLAG_BFL:
				cny_state = CNY_ST_FL;
				move(DROITE);
				break;

			case FLAG_BFR:
				move(GAUCHE);
				cny_state = CNY_ST_FR;
				break;

			case FLAG_BBL:
				move(AVANT_DROITE);
				cny_state = CNY_ST_BL;
				break;

			case FLAG_BBR:
				move(AVANT_GAUCHE);
				cny_state = CNY_ST_BR;
				break;

			// 2 capteurs
			case FLAG_BFL | FLAG_BFR:
				switch (cny_state) {
					case CNY_ST_FL_R:
					case CNY_ST_FR_L:
						break;

					case CNY_ST_FL:
						cny_state = CNY_ST_FL_R,
						move(DROITE);
						break;

					case CNY_ST_NONE: // par défaut : on tourne à gauche
					case CNY_ST_FR:
					default:
						cny_state = CNY_ST_FR_L,
						move(GAUCHE);
						break;
				}
				break;

			case FLAG_BFL | FLAG_BBL:
				move(AVANT_DROITE);
				cny_state = CNY_ST_LALL;
				break;

			case FLAG_BFR | FLAG_BBR:
				move(AVANT_GAUCHE);
				cny_state = CNY_ST_RALL;
				break;

			case FLAG_BBL | FLAG_BBR:
				switch (cny_state) {
					case CNY_ST_BL_R:
					case CNY_ST_BR_L:
						break;

					case CNY_ST_BL:
						cny_state = CNY_ST_BL_R;
						move(AVANT_DROITE);
						break;

					case CNY_ST_NONE: // par défaut : on tourne à droite
					case CNY_ST_BR:
					default:
						cny_state = CNY_ST_BR_L,
						move(AVANT_GAUCHE);
						break;
				}
				break;
			
			// 3 ou 4 capteurs:
			case FLAG_BBL | FLAG_BBR | FLAG_BFL:
				move(AVANT_DROITE);
				break;

			case FLAG_BBL | FLAG_BBR | FLAG_BFR:
				move(AVANT_GAUCHE);
				break;

			case FLAG_BBL | FLAG_BBR | FLAG_BFL | FLAG_BFR:
				if (cny_state == CNY_ST_OUTSIDE) {
					if (millis() - cny_state_timer > 4000) {
						move(STOP);
						while(true) {
							// si on est entièrement dehors depuis plus de 4 secondes, alors on s'arrête
							digitalWrite(LED_BUILTIN, HIGH);
							delay(250);
							digitalWrite(LED_BUILTIN, LOW);
							delay(250);
						}
					}
				} else {
					cny_state = CNY_ST_OUTSIDE;
					cny_state_timer = 0;
				}
				break;

			default:
				break;
		}
  } else {
		if (cny_state != CNY_ST_NONE) {
			switch (cny_state) {
				// une fois le recentrage terminé
				case CNY_POST_STL:
				case CNY_POST_STR:
					if (millis() - cny_state_timer > CNY_POST_DELAY) {
						cny_state = CNY_ST_NONE;
						cny_state_timer = 0;
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

/// Fonctions utilitaires

// déplacement
void move(Direction direction) {
  switch (direction) {
		case AVANT:
			servo_r.writeMicroseconds(SR_AVANT);
			servo_l.writeMicroseconds(SL_AVANT);
			break;
		case AVANT_MOYEN:
			servo_r.writeMicroseconds(SR_AVANT_LENT);
			servo_l.writeMicroseconds(SL_AVANT_LENT);
			break;
		case AVANT_GAUCHE:
			servo_r.writeMicroseconds(SR_AVANT);
			servo_l.writeMicroseconds(SL_AVANT_LENT);
			break;
		case AVANT_GAUCHE_FORT:
			servo_r.writeMicroseconds(SR_AVANT);
			servo_l.writeMicroseconds(SL_STOP);
			break;
		case AVANT_DROITE:
			servo_r.writeMicroseconds(SR_AVANT_LENT);
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
			servo_r.writeMicroseconds(SR_ARRIERE_LENT);
			servo_l.writeMicroseconds(SL_ARRIERE_LENT);
			break;
		case ARRIERE_GAUCHE:
			servo_r.writeMicroseconds(SR_ARRIERE);
			servo_l.writeMicroseconds(SL_ARRIERE_LENT);
			break;
		case ARRIERE_GAUCHE_FORT:
			servo_r.writeMicroseconds(SR_ARRIERE);
			servo_l.writeMicroseconds(SL_STOP);
			break;
		case ARRIERE_DROITE:
			servo_r.writeMicroseconds(SR_ARRIERE_LENT);
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

// mesure de distance
float distance(Vision vision) {
	float duration_us, distance_cm;
	int pin_echo, pin_trig;
  
	switch (vision) {
    case VISION_GAUCHE:
      pin_echo = PIN_TL_ECH;
      pin_trig = PIN_TL_TRG;
     break;
    case VISION_CENTRE:
      pin_echo = PIN_TC_ECH;
      pin_trig = PIN_TC_TRG;
     break;
    case DROITE:
      pin_echo = PIN_TR_ECH;
      pin_trig = PIN_TR_TRG;
     break;
	}
	
	digitalWrite(pin_trig, HIGH);
	delayMicroseconds(10);
	digitalWrite(pin_trig, LOW);

	duration_us = pulseIn(pin_echo, HIGH, 10000);
	distance_cm = 0.017 * duration_us;
	
	return(distance_cm);
}