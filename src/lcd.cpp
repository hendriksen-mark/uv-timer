#include "lcd.h"
#include "globals.h"

void lcdPrintTime(uint8_t mm, uint8_t ss)
{
  char buf[6];
  snprintf(buf, sizeof(buf), "%02u:%02u", mm, ss);
  lcd.print(buf);
}

void showStartupMessage()
{
  lcd.clear();
  lcd.printCenter("UV Exposure Syst", 0);
  lcd.printCenter("Waveshare RP2040", 1);
  delay(2000);

  lcd.clear();
  lcd.printCenter("Original by", 0);
  lcd.printCenter("Stynus (PICBASIC)", 1);
  delay(2000);
  lcd.clear();
}

void drawStartScreen()
{
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcdPrintTime(timeMin, timeSec);
  lcd.print("      ");

  lcd.setCursor(0, 1);
  lcd.print("Start");
  if (doubleSideEnabled)
  {
    lcd.print(singleDouble ? "|D.S.|" : "|S.S.|");
  }
  else
  {
    lcd.print("|     |");
  }
  lcd.print("SetT");
}
