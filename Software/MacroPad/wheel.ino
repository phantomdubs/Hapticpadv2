void twistScroll(){
  if(wheelModeChanged){
    targetAngle = encoder.getAngle();
    motor.controller = MotionControlType::angle;
    motor.PID_velocity.I = Twist_I; //0.2;
    Serial.print("Set I = ");
    Serial.println(Twist_I);
    motor.PID_velocity.P = Twist_P; //0.65f;
    Serial.print("Set P = ");
    Serial.println(Twist_P);
    motor.enable();
    wheelModeChanged = false;
  }

  encoderAngle = encoder.getAngle();

  motor.move(targetAngle);

  interval = 50 / abs(encoderAngle - targetAngle);

  if(abs(encoderAngle - targetAngle) > 0.2){ 
    if(mouseTimer + interval < millis()){
      wheelActionCheck();
      if(encoderAngle > targetAngle){
        usb_mouse.mouseScroll(0, -1, 0);
      } else {
        usb_mouse.mouseScroll(0, 1, 0);
      }
      mouseTimer = millis();
    } else {
      cancelWheelAction();
    }
  }
}
/*void twistScroll(){ //Uncommment for consant movement instead of twist mode
  if(wheelModeChanged){
    //targetAngle = encoder.getAngle();
    motor.controller = MotionControlType::velocity;
    motor.enable();
    wheelModeChanged = false;
  }

  motor.move(1);

}*/

void notchyWheel(){
  int scroll;

  if(wheelModeChanged){
    motor.controller = MotionControlType::angle;
    motor.PID_velocity.I = Clicky_I;//0;
    Serial.print("Set I = ");
    Serial.println(Clicky_I);
    motor.PID_velocity.P = Clicky_P;//0.5f;
    Serial.print("Set P = ");
    Serial.println(Clicky_P);
    motor.enable();
    wheelModeChanged = false;
  }


  motor.move(target_angle);
  encoderAngle = encoder.getAngle(); //6

  new_target_angle = round(encoderAngle / angle_step) * angle_step;

  if(new_target_angle != target_angle){
    wheelActionCheck();
    scroll = round((target_angle - new_target_angle) / angle_step);
    if(!profileSelectMenu){
      usb_mouse.mouseScroll(0, scroll, 0);
    } else {
      if(scroll < 0){
        if(activeProfile < totalProfiles - 1){
          activeProfile++;
        }
      } else {
        if(activeProfile > 0){
          activeProfile--;
        }
      }
    }
    target_angle = new_target_angle;
    motor.move(target_angle);
  } else {
    cancelWheelAction();
  }
}

void wheelActionCheck(){
  wheelKeyTimer = millis();
  if(wheelAction != 0 && !wheelKeyPressed){
    int modifier = checkModifiers(wheelAction);
    uint8_t keycode[6] = { 0 };
    if(modifier != 0){
      usb_keyboard.keyboardReport(0, modifier, 0);
    } else {
      keycode[0] = convertKeycode(wheelAction);
      usb_keyboard.keyboardReport(0, 0, keycode);
    }
    wheelKeyPressed = true;
    delay(10); //Slight delay to ensure keyboard report has been sent before mouse move
  }
}

void cancelWheelAction(){
  if(wheelKeyPressed && wheelKeyTimer + 100 < millis()){
    usb_keyboard.keyboardReport(0, 0, 0);
    wheelKeyPressed = false;
  }
}

void freeSpinning(){
  if(wheelModeChanged){
    motor.controller = MotionControlType::velocity;
    motor.PID_velocity.I = Momentum_I; //0;
    motor.PID_velocity.P = Momentum_P; //0.3f;
    wheelModeChanged = false;
  }

  //motor.disable();

  float vel = encoder.getVelocity(); //Read current encoder velocity

  if(!decelerating){
    if(abs(last_velocity - vel) > 0.1){//Detected deceleration
      if(decelDetected){
        if(debounceTimer + 100 < millis()){
          last_velocity = vel;
          if(vel > 0){
            target_velocity = vel + 2; //Set current velocity as target
          } else {
            target_velocity = vel - 2; //Set current velocity as target
          }
          if(abs(target_velocity) > 6){
            motor.enable();
          }
          debounceTimer = millis();
        }
      } else {
        decelDetected = true;
        debounceTimer = millis();
      }
    } else {
      decelDetected = false;
      last_velocity = vel;
    }
  }

  if(motor.enabled){
    if(vel > 0.2){ //Moving in the positive direction
      if(aceltimer + 400 < micros()){
        target_velocity -= 0.02;
        aceltimer = micros();
        decelerating = true;
      }
    } else if(vel < -0.2){ //Moving in the negative direction
      if(aceltimer + 400 < micros()){
        target_velocity += 0.02;
        aceltimer = micros();
        decelerating = true;
      }
    } else {
      motor.disable();

      encoderAngle = encoder.getAngle();
      lastEncoderAngle = encoderAngle;

      decelerating = false;
      debounceTimer = millis();
      last_velocity = vel;
      target_velocity = 0;
    }
    motor.move(target_velocity);
  }
}
