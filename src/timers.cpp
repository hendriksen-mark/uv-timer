#include "timers.h"
#include "globals.h"

#if !defined(ARDUINO_ARCH_RP2040)
#error "This project is configured for RP2040 only."
#endif

#include <pico/time.h>

static repeating_timer_t g_timer1ms;
static bool g_timerStarted = false;

static bool onTimer1ms(repeating_timer_t *)
{
  if (!timerGo)
  {
    return true;
  }

  ++msAccumulator;
  if (msAccumulator < tickMs)
  {
    return true;
  }

  msAccumulator = 0;

  if (countSec > 0)
  {
    --countSec;
    return true;
  }

  if (countMin > 0)
  {
    --countMin;
    countSec = 59;
    return true;
  }

  timerGo = false;
  return true;
}

void updateTickFromCalibration()
{
  // TIMER_CALIBRATION_CENTER (128) means exactly 1000 ms.
  // Lower values speed up, higher values slow down.
  int16_t offset = static_cast<int16_t>(calByte) - TIMER_CALIBRATION_CENTER;
  int16_t candidate = 1000 + offset;
  if (candidate < MIN_TICK_MS)
  {
    candidate = MIN_TICK_MS;
  }
  if (candidate > MAX_TICK_MS)
  {
    candidate = MAX_TICK_MS;
  }
  noInterrupts();
  tickMs = static_cast<uint16_t>(candidate);
  interrupts();
}

void setupTimer1()
{
  if (g_timerStarted)
  {
    return;
  }

  if (add_repeating_timer_ms(1, onTimer1ms, nullptr, &g_timer1ms))
  {
    g_timerStarted = true;
  }
}
