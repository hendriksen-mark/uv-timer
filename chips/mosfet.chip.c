// Wokwi Custom Chip - For docs and examples see:
// https://docs.wokwi.com/chips-api/getting-started
//
// SPDX-License-Identifier: MIT
// Copyright 2023 Christian Tastot

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  pin_t pin_drain;
  pin_t pin_source;
  pin_t pin_gate;
} chip_state_t;

static void chip_pin_change(void *user_data, pin_t pin, uint32_t value) {
  chip_state_t *chip = (chip_state_t*)user_data;

  if (value == HIGH && pin_read(chip->pin_source) == LOW) {
    pin_mode(chip->pin_drain, INPUT);
  } else {
    pin_mode(chip->pin_drain, OUTPUT);
  }
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->pin_drain = pin_init("DRAIN", OUTPUT);
  chip->pin_source = pin_init("SOURCE", INPUT);
  chip->pin_gate = pin_init("GATE", INPUT);

  const pin_watch_config_t config = {
    .edge = BOTH,
    .pin_change = chip_pin_change,
    .user_data = chip,
  };
  pin_watch(chip->pin_gate, &config);
}
  //printf("Hello from custom chip!\n");
//}
