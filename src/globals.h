#pragma once

#include <pebble.h>

// Layout
#define MARGIN 5
#define THICKNESS 3
#define ANIMATION_DELAY 300
#define ANIMATION_DURATION 1000
#define HAND_LENGTH_SEC 65
#define HAND_LENGTH_MIN 65
#define HAND_LENGTH_HOUR 45

#define PERSIST_DEFAULTS_SET 3489

// Persist
#define PERSIST_KEY_DATE        0
#define PERSIST_KEY_DAY         1
#define PERSIST_KEY_BT          2
#define PERSIST_KEY_BATTERY     3
#define PERSIST_KEY_SECOND_HAND 4
#define NUM_SETTINGS            5

typedef struct {
  int days;
  int hours;
  int minutes;
  int seconds;
} Time;
