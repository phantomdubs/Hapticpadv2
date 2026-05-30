// Animation speed controls - adjust these values to change animation speeds
unsigned long bandSpeed = 200; // Higher number = slower alternation. 200 means it toggles colors every 200ms.
unsigned long breathSpeed = 15; // Higher number = slower breathing. 15 means it updates the brightness every 15ms.
unsigned long haloSpeed = 80; // Higher number = slower animation. 40 means it moves 1 pixel every 40ms.
unsigned long lastBandTime = 0;
unsigned long lastBreathTime = 0;
unsigned long lastHaloTime = 0;

void ledBand(){
  if (millis() - lastBandTime >= bandSpeed) {
    lastBandTime = millis(); // Reset the timer
    
    if(evenNumber){
      evenNumber = false;
    } else {
      evenNumber = true;
    }
  }

  for(int i = 0; i < NUM_LEDS; i++){
    if(evenNumber){
      if(i % 2){
        leds[i].red = primaryColour[0] - (redScale * sequenceStep);
        leds[i].green = primaryColour[1] - (greenScale * sequenceStep);
        leds[i].blue = primaryColour[2] - (blueScale * sequenceStep);
      } else {
        leds[i].red = secondaryColour[0] - (redScale * sequenceStep);
        leds[i].green = secondaryColour[1] - (greenScale * sequenceStep);
        leds[i].blue = secondaryColour[2] - (blueScale * sequenceStep);
      }
    } else {
      if(i % 2){
        leds[i].red = secondaryColour[0] - (redScale * sequenceStep);
        leds[i].green = secondaryColour[1] - (greenScale * sequenceStep);
        leds[i].blue = secondaryColour[2] - (blueScale * sequenceStep);
      } else {
        leds[i].red = primaryColour[0] - (redScale * sequenceStep);
        leds[i].green = primaryColour[1] - (greenScale * sequenceStep);
        leds[i].blue = primaryColour[2] - (blueScale * sequenceStep);
      }
    }
  }
  FastLED.setBrightness(ledBrightness);
  FastLED.show();
  loopCounter++;
}

void breathLED(){
  if (millis() - lastBreathTime >= breathSpeed) {
    lastBreathTime = millis(); // Reset the timer
    
    if(breathIncrease){
      if(sequenceStep < 100){
        sequenceStep++;
      } else {
        breathIncrease = false;
      }
    } else {
      if(sequenceStep > 0){
        sequenceStep--;
      } else {
        breathIncrease = true;
      }
    }
  }

  for(int i = 0; i < NUM_LEDS; i++){
    leds[i].red = primaryColour[0] - (redScale * sequenceStep);
    leds[i].green = primaryColour[1] - (greenScale * sequenceStep);
    leds[i].blue = primaryColour[2] - (blueScale * sequenceStep);
    //colors[i] = rgb_color(primaryColour[0] - (redScale * sequenceStep),primaryColour[1] - (greenScale * sequenceStep),primaryColour[2] - (blueScale * sequenceStep));
  }
  FastLED.setBrightness(ledBrightness);
  FastLED.show();
  //ledStrip.write(colors, ledCount, ledBrightness);
}

void haloLED(){
  
  for(int i = 0; i < NUM_LEDS; i++){
    if(i == haloCount){
      sequenceStep = 0;
    }
    leds[i].red = primaryColour[0] - (redScale * sequenceStep);
    leds[i].green = primaryColour[1] - (greenScale * sequenceStep);
    leds[i].blue = primaryColour[2] - (blueScale * sequenceStep);
    sequenceStep++;
  }
  // Only advance the frame position when the speed time has passed
  if (millis() - lastHaloTime >= haloSpeed) {
    lastHaloTime = millis(); // Reset the timer
    
    if(haloCount < NUM_LEDS - 1){ // Use NUM_LEDS - 1 to prevent out-of-bounds math
      haloCount++; 
    } else {
      haloCount = 0;
    }
  }

  FastLED.setBrightness(ledBrightness);
  FastLED.show();
}

void solidLED(){
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i].red = primaryColour[0];
    leds[i].green = primaryColour[1];
    leds[i].blue = primaryColour[2];
  }

  FastLED.setBrightness(ledBrightness);
  FastLED.show();
}

void offLED(){
  FastLED.setBrightness(0);
  FastLED.show();
}

void rainbowLED(){
  uint8_t time = millis() >> 4;

  for(uint16_t i = 0; i < NUM_LEDS; i++)
  {
    uint8_t p = time - i * ledSpeed;
    leds[i] = CHSV((uint32_t)p * 359 / 256, 255, 255);
  }

  FastLED.setBrightness(ledBrightness);
  FastLED.show();
}

void calculateColourMultiplier(){
  int steps = 1; // <--- Default to 1 to prevent division-by-zero crashes!
  
  if(ledMode == 0){
    steps = 20;
  } else if(ledMode == 1){
    steps = 100;
  } else if(ledMode == 2){
    steps = 10;
  }
  
  redScale = primaryColour[0] - secondaryColour[0];
  redScale = redScale / steps; 

  greenScale = primaryColour[1] - secondaryColour[1];
  greenScale = greenScale / steps;

  blueScale = primaryColour[2] - secondaryColour[2];
  blueScale = blueScale / steps; 
}