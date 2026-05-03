// --- display.ino ---



void loadButtonIcons(){
  char filePath[maxProfileNameLength + 6];

  snprintf(filePath, sizeof(filePath), "/%s/1.bmp", profileName);
  loadBMP24x24(filePath, icon1);

  snprintf(filePath, sizeof(filePath), "/%s/2.bmp", profileName);
  loadBMP24x24(filePath, icon2);

  snprintf(filePath, sizeof(filePath), "/%s/3.bmp", profileName);
  loadBMP24x24(filePath, icon3);

  snprintf(filePath, sizeof(filePath), "/%s/4.bmp", profileName);
  loadBMP24x24(filePath, icon4);

  snprintf(filePath, sizeof(filePath), "/%s/5.bmp", profileName);
  loadBMP24x24(filePath, icon5);

  snprintf(filePath, sizeof(filePath), "/%s/6.bmp", profileName);
  loadBMP24x24(filePath, icon6);
}

void drawGrid() { 
  u8g2.drawLine(10, 0, 10, 64); 
  u8g2.drawLine(10, 32, 128, 32); 
  u8g2.drawLine(49, 0, 49, 64);
  u8g2.drawLine(88, 0, 88, 64);
}

void drawActiveProfile(){ 
  drawGrid();

  // 1. STATIC VERTICAL PROFILE NAME (90° CCW)
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.setFontDirection(3); // Rotates text 90 degrees counter-clockwise
  
  // x=7 (horizontal position)
  // y=60 (starts near the bottom and draws "up")
  u8g2.drawStr(7, 60, profileName); 
  
  u8g2.setFontDirection(0); // Reset back to normal for icons and labels

  // 2. Draw Icons
  drawIcon24x24(17, 2, icon1);
  drawIcon24x24(56, 2, icon2);  
  drawIcon24x24(95, 2, icon3);

  drawIcon24x24(17, 34, icon4);
  drawIcon24x24(56, 34, icon5);
  drawIcon24x24(95, 34, icon6);

  // 3. Draw Labels
  u8g2.setFont(u8g2_font_u8glib_4_tf);
  int colX[3] = {30, 69, 108};
  int rowY[2] = {31, 63};
  for(int i = 0; i < 6; i++) {
    int col = i % 3;
    int row = i / 3;
    int labelX = colX[col] - (u8g2.getStrWidth(buttonLabel[i]) / 2);
    u8g2.drawStr(labelX, rowY[row], buttonLabel[i]);
  }
}

void drawProfileMenu(){
  int y = 34 - activeProfile * 9;
  u8g2.setFont(u8g2_font_5x7_tf);
  for(int i = 0; i < totalProfiles; i++){
    if(i == activeProfile){
      u8g2.drawBox(0, y - 8, u8g2.getStrWidth(profileNames[i]) + 2, 9);
      u8g2.setDrawColor(0); 
      u8g2.drawStr( 1, y, profileNames[i]);
      u8g2.setDrawColor(1); 
    } else if(y > 0 && y < 70){
      u8g2.drawStr( 1, y, profileNames[i]);
    }
    y += 9;
  }
}