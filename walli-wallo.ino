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

// constantes capteurs ultrasons (Tx et HC_xxx)
#define PIN_TR_TRG 1
#define PIN_TR_ECH 0
#define PIN_TC_TRG 2
#define PIN_TC_ECH 3
#define PIN_TL_TRG 5
#define PIN_TL_ECH 4

#define HC_SEUIL_LOIN 4706
#define HC_SEUIL_PROCHE 1177

#define FLAG_PROCHE 0b101010
#define FLAG_LOIN   0b010101
#define FLAG_TL     0b110000
#define FLAG_TC     0b001100
#define FLAG_TR     0b000011

#define HC_MESURE_DELAY 200

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
  AVANT_LENT,
  AVANT_GAUCHE,
  AVANT_GAUCHE_FORT,
  AVANT_DROITE,
  AVANT_DROITE_FORT,

  ARRIERE,
  ARRIERE_LENT,
  ARRIERE_GAUCHE,
  ARRIERE_GAUCHE_FORT,
  ARRIERE_DROITE,
  ARRIERE_DROITE_FORT,

  GAUCHE,
  DROITE,

  STOP,
};

// position capteurs ultrasons
enum Vision {
    VISION_DROIT,
    VISION_GAUCHE,
    VISION_CENTRE,
};

// flags des capteurs ultrasons et infrarouges
typedef byte PositionFlags;
typedef byte CnyFlags;

// état de l'exécution
enum State {
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

  CNY_ST_OUTSIDE,

  ATTAQUE,
  RECHERCHE,
};

/// Fonctions et variables principales

// variables générales
Servo servo_r, servo_l;
State state = RECHERCHE;
unsigned long state_timer = 0;
PositionFlags adversaire = 0;

// fonctions de mode
void handle_cny(CnyFlags cny_flags) {
  // pousser jusqu'au bout si on est en mode attaque
  if (state == ATTAQUE) {
    switch (cny_flags) {
      case FLAG_BFL:
      case FLAG_BFR:
      case FLAG_BFL | FLAG_BFR:
        handle_attaque();
        return;
    }
  }

  switch(cny_flags) {

    // 1 capteur
    case FLAG_BFL:
      state = CNY_ST_FL;
      move(DROITE);
      break;

    case FLAG_BFR:
      move(GAUCHE);
      state = CNY_ST_FR;
      break;

    case FLAG_BBL:
      move(AVANT_DROITE);
      state = CNY_ST_BL;
      break;

    case FLAG_BBR:
      move(AVANT_GAUCHE);
      state = CNY_ST_BR;
      break;

    // 2 capteurs
    case FLAG_BFL | FLAG_BFR:
      switch (state) {
        case CNY_ST_FL_R:
        case CNY_ST_FR_L:
          break;

        case CNY_ST_FL:
          state = CNY_ST_FL_R,
          move(DROITE);
          break;

        case RECHERCHE:
        case ATTAQUE: // par défaut : on tourne à gauche
        case CNY_ST_FR:
        default:
          state = CNY_ST_FR_L,
          move(GAUCHE);
          break;
      }
      break;

    case FLAG_BFL | FLAG_BBL:
      move(AVANT_DROITE);
      state = CNY_ST_LALL;
      break;

    case FLAG_BFR | FLAG_BBR:
      move(AVANT_GAUCHE);
      state = CNY_ST_RALL;
      break;

    case FLAG_BBL | FLAG_BBR:
      switch (state) {
        case CNY_ST_BL_R:
        case CNY_ST_BR_L:
          break;

        case CNY_ST_BL:
          state = CNY_ST_BL_R;
          move(AVANT_DROITE);
          break;

        case RECHERCHE:
        case ATTAQUE: // par défaut : on tourne à droite
        case CNY_ST_BR:
        default:
          state = CNY_ST_BR_L,
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
      if (state == CNY_ST_OUTSIDE) {
        if (millis() - state_timer > 4000) {
          move(STOP);
          while(true) {
            // si on est entièrement dehors depuis plus de 4 secondes, alors on s'arrête
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(50);
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(250);
          }
        }
      } else {
        state = CNY_ST_OUTSIDE;
        state_timer = millis();
      }
      break;

    default:
      break;
  }
}

void handle_recentrage() {
  switch (state) {
    // une fois le recentrage terminé
    case CNY_POST_STL:
    case CNY_POST_STR:
      if (millis() - state_timer > CNY_POST_DELAY) {
        state = RECHERCHE;
        state_timer = millis();
      }
      break;

    // on revient vers le centre à droite
    case CNY_ST_FL:
    case CNY_ST_BL:
    case CNY_ST_LALL:
      state = CNY_POST_STL;
      state_timer = millis();
      move(AVANT_DROITE_FORT);
      break;

    // par défaut
    // on revient vers le centre à gauche
    case CNY_ST_FR:
    case CNY_ST_BR:
    case CNY_ST_RALL:
      state = CNY_POST_STR;
      state_timer = millis();
      move(AVANT_GAUCHE_FORT);
      break;
  }
}

void handle_attaque() {
  seek_adversaire();

  switch (adversaire) {
    // zone 1 : loin gauche
    case FLAG_LOIN & FLAG_TL:
      move(GAUCHE);
      break;

    // zone 2 : loin centre-gauche
    case FLAG_LOIN & FLAG_TC:
    case FLAG_LOIN & (FLAG_TL | FLAG_TC):
      move(AVANT_DROITE);
      break;

    // zone 3 : loin droite et centre-droite
    case FLAG_LOIN & FLAG_TR:
    case FLAG_LOIN & (FLAG_TC | FLAG_TR):
      move(AVANT);
      break;

    // zone 4 : proche gauche et centre-gauche
    case FLAG_PROCHE & FLAG_TL:
    case FLAG_PROCHE & (FLAG_TL | FLAG_TC):
    case (FLAG_PROCHE & FLAG_TL) | (FLAG_LOIN & FLAG_TC):
    case (FLAG_LOIN & FLAG_TL) | (FLAG_PROCHE & FLAG_TC):
      move(GAUCHE);
      break;

    // zone 5 : proche centre
    case FLAG_PROCHE & FLAG_TC:
      move(AVANT_LENT);
      break;

    // zone 6 : proche droite et centre-droite
    case FLAG_PROCHE & FLAG_TR:
    case FLAG_PROCHE & (FLAG_TC | FLAG_TR):
    case (FLAG_LOIN & FLAG_TC) | (FLAG_PROCHE & FLAG_TR):
    case (FLAG_PROCHE & FLAG_TC) | (FLAG_LOIN & FLAG_TR):
      move(AVANT_DROITE_FORT);
      break;

    // autre, incohérent ou inconnu
    case 0:
    default:
      move(DROITE);
      break;
  }
}

void handle_recherche() {
  seek_adversaire();

  switch (adversaire) {
    // zone 1
    case FLAG_LOIN & FLAG_TL:
      state = ATTAQUE;
    // zone 2
    case FLAG_LOIN & FLAG_TC:
    case FLAG_LOIN & (FLAG_TL | FLAG_TC):
      move(GAUCHE);
      break;

    // zone 3
    case FLAG_LOIN & FLAG_TR:
    case FLAG_LOIN & (FLAG_TC | FLAG_TR):
      move(AVANT);
      state = ATTAQUE;
      break;

    // zone 4
    case FLAG_PROCHE & FLAG_TL:
    case FLAG_PROCHE & (FLAG_TL | FLAG_TC):
    case (FLAG_PROCHE & FLAG_TL) | (FLAG_LOIN & FLAG_TC):
    case (FLAG_LOIN & FLAG_TL) | (FLAG_PROCHE & FLAG_TC):
      move(GAUCHE);
      break;

    // zone 5
    case FLAG_PROCHE & FLAG_TC:
      move(AVANT);
      state = ATTAQUE;
      break;

    // zone 6
    case FLAG_PROCHE & FLAG_TR:
    case FLAG_PROCHE & (FLAG_TC | FLAG_TR):
    case (FLAG_LOIN & FLAG_TC) | (FLAG_PROCHE & FLAG_TR):
    case (FLAG_PROCHE & FLAG_TC) | (FLAG_LOIN & FLAG_TR):
      move(DROITE);
      state = ATTAQUE;
      break;

    // autre, incohérent ou inconnu
    case 0:
    default:
      move(GAUCHE);
      break;
  }
}

// fonctions de base
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
  CnyFlags cny_flags = 0;
  if (CNY_READ(PIN_BFL)) cny_flags |= FLAG_BFL;
  if (CNY_READ(PIN_BFR)) cny_flags |= FLAG_BFR;
  if (CNY_READ(PIN_BBL)) cny_flags |= FLAG_BBL;
  if (CNY_READ(PIN_BBR)) cny_flags |= FLAG_BBR;

  if (cny_flags != 0) {
    handle_cny(cny_flags);
  } else {
    switch (state) {
      case RECHERCHE:
        handle_recherche();
        break;

      case ATTAQUE:
        handle_attaque();
        break;

      // recentrage
      default:
        handle_recentrage();
        break;
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
    case AVANT_LENT:
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
    case ARRIERE_LENT:
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

void seek_adversaire() {
  if (millis() - state_timer > HC_MESURE_DELAY) {
    move(STOP);
    state_timer = millis();

    adversaire = 0;
    adversaire |= position(VISION_GAUCHE) & FLAG_TL;
    adversaire |= position(VISION_CENTRE) & FLAG_TC;
    adversaire |= position(VISION_DROIT) & FLAG_TR;
  }
}

PositionFlags position(Vision vision) {
  int pin_echo, pin_trigger;
  switch (vision) {
    case VISION_GAUCHE:
      pin_echo = PIN_TL_ECH;
      pin_trigger = PIN_TL_TRG;
      break;

    case VISION_CENTRE:
      pin_echo = PIN_TC_ECH;
      pin_trigger = PIN_TC_TRG;
      break;

    case VISION_DROIT:
      pin_echo = PIN_TR_ECH;
      pin_trigger = PIN_TR_TRG;
      break;
  }

  digitalWrite(pin_trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(pin_trigger, LOW);
  float duration_us = pulseIn(pin_echo, HIGH, HC_SEUIL_LOIN);

  if ((HC_SEUIL_PROCHE < duration_us) && (duration_us < HC_SEUIL_LOIN))
    return(FLAG_LOIN);

  else if ((0 < duration_us ) && (duration_us < HC_SEUIL_PROCHE))
    return(FLAG_PROCHE);

  else
    return(0);
}
