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

#define HC_FRONTIERE_HAUT 4706
#define HC_FRONTIERE_BAS 1177

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

enum Distance {
  RIEN,
  PROCHE,
  LOIN,
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
	CNY_OUTSIDE,
};

enum Etat_ia {
  ATTAQUE,
  RECHERCHE,
};

Etat_ia etat_ia = RECHERCHE;
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
					case CNY_ST_FL_R:
					case CNY_ST_FR_L:
						break;

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
					case CNY_ST_BL_R:
					case CNY_ST_BR_L:
						break;

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
			case CNY_FL_BL | CNY_FL_BR | CNY_FL_FL:
				move(AVANT_DROITE);
				break;

			case CNY_FL_BL | CNY_FL_BR | CNY_FL_FR:
				move(AVANT_GAUCHE);
				break;

			case CNY_FL_BL | CNY_FL_BR | CNY_FL_FL | CNY_FL_FR:
				if (cny_state == CNY_OUTSIDE) {
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
					cny_state = CNY_OUTSIDE;
					cny_state_timer = 0;
				}
				break;

			default:
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
       Distance DIST_GAUCHE = distance(VISION_GAUCHE);
       Distance DIST_CENTRE = distance(VISION_CENTRE);
       Distance DIST_DROITE = distance(VISION_DROIT);
 
       switch(DIST_GAUCHE + DIST_CENTRE*3 + DIST_DROITE*9){
         //1 
         case 2:
           if (etat_ia = RECHERCHE) {
             move(GAUCHE);
           } else {
             move(GAUCHE);
             etat_ia = ATTAQUE;
           }
           break;
           
        //2 
        case 6:
        case 8:
           if (etat_ia = RECHERCHE) {
             move(GAUCHE);
           } else {
             move(AVANT_DROITE);
           }
           break;
           
         //3 
        case 18:
        case 24:
           if (etat_ia = RECHERCHE) {
             move(AVANT);
             etat_ia = ATTAQUE;
           } else {
             move(AVANT);
           }
          break;
        
        //4 
        case 1:
        case 4:
        case 5:
        case 7:
           move(GAUCHE);
           break;
        
        //5 
        case 3:
          if (etat_ia = RECHERCHE) {
             move(AVANT);
             etat_ia = ATTAQUE;
           } else {
             move(AVANT_MOYEN);
           }
          break;
        
        //6 
        case 9:
        case 12:
        case 15:
        case 21:
          if (etat_ia = RECHERCHE) {
             move(DROITE);
             etat_ia = ATTAQUE;
           } else {
             move(AVANT_DROITE_FORT);
           }
           break;
        
        //0
        default:
          if (etat_ia = RECHERCHE) {
            move(GAUCHE);
          } else {
            move(DROITE);
          }
          break;
      }
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

Distance distance(Vision vision) {
float duration_us;
int T_ECH, T_TRG;
    switch (vision) {
    case VISION_GAUCHE:
       T_ECH = TL_ECH;
       T_TRG = TL_TRG;
     break;
    case VISION_CENTRE:
       T_ECH = TC_ECH;
       T_TRG = TC_TRG;
     break;
    case VISION_DROIT:
       T_ECH = TR_ECH;
       T_TRG = TR_TRG;
     break;
}
  digitalWrite(T_TRG, HIGH);
  delayMicroseconds(10);
  digitalWrite(T_TRG, LOW);


  duration_us = pulseIn(T_ECH, HIGH);
  if((HC_FRONTIERE_BAS < duration_us) && (duration_us < HC_FRONTIERE_HAUT)){
    return(LOIN);
  }
  else if((0 < duration_us ) && (duration_us < HC_FRONTIERE_BAS)){
    return(PROCHE);
  }
  else {
    return(RIEN);
    
  }
 }

