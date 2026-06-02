import re

with open('/Users/thaongan/Downloads/CarLine/CarLine.ino', 'r') as f:
    text = f.read()

# Fix turnLeft
orig_left = """void turnLeft(){
  unsigned long startTime = millis();
  setMotor(-TURN_SPEED, TURN_SPEED);

  while(digitalRead(S3)==0){
    webSocket.loop();
    server.handleClient();

    if(millis() - startTime > 1000){
      break;
    }
  }

  stopMotor();
  delay(100);
}"""

new_left = """void turnLeft(){
  setMotor(-TURN_SPEED, TURN_SPEED);
  
  // 1. Xoay mù 300ms để S3 lệch khỏi vạch ngã tư hiện tại
  unsigned long t_blind = millis();
  while(millis() - t_blind < 300) { server.handleClient(); webSocket.loop(); }

  // 2. Tiếp tục xoay tìm vạch mới
  unsigned long startTime = millis();
  while(digitalRead(S3)==0){
    webSocket.loop();
    server.handleClient();
    if(millis() - startTime > 1500) break; // timeout
  }

  stopMotor();
  delay(100);
}"""
text = text.replace(orig_left, new_left)

# Fix turnRight
orig_right = """void turnRight(){
  unsigned long startTime = millis();
  setMotor(TURN_SPEED, -TURN_SPEED);

  while(digitalRead(S3)==0){
    webSocket.loop();
    server.handleClient();

    if(millis() - startTime > 1000){
      break;
    }
  }

  stopMotor();
  delay(100);
}"""

new_right = """void turnRight(){
  setMotor(TURN_SPEED, -TURN_SPEED);

  // 1. Xoay mù 300ms để S3 lệch khỏi vạch ngã tư hiện tại
  unsigned long t_blind = millis();
  while(millis() - t_blind < 300) { server.handleClient(); webSocket.loop(); }

  // 2. Tiếp tục xoay tìm vạch mới
  unsigned long startTime = millis();
  while(digitalRead(S3)==0){
    webSocket.loop();
    server.handleClient();
    if(millis() - startTime > 1500) break;
  }

  stopMotor();
  delay(100);
}"""
text = text.replace(orig_right, new_right)

# Fix loop and debounce
orig_loop = """  if(moving && pathLength > 1){
    followLine();

    if(isNode() && millis() - lastNodeTime > 500){
      lastNodeTime = millis();

      int from = path[currentIndex];
      int to = path[currentIndex + 1];

      String dir = getDir(from,to);

      sendNode(to);

      if(dir=="LEFT"){
        turnLeft();
      }
      else if(dir=="RIGHT"){
        turnRight();
      }

      currentIndex++;"""

new_loop = """  if(moving && pathLength > 1){
    followLine();

    // Tăng thời gian cooldown giữa 2 node liên tiếp lên 1200ms để chống dội (bouncing)
    if(isNode() && millis() - lastNodeTime > 1200){

      int from = path[currentIndex];
      int to = path[currentIndex + 1];

      String dir = getDir(from,to);

      sendNode(to);

      if(dir=="LEFT"){
        turnLeft();
      }
      else if(dir=="RIGHT"){
        turnRight();
      }
      else {
        // Đi thẳng: lao mù trong 200ms để chắc chắn đuôi xe thoát khỏi vạch ngang ngã tư
        setMotor(SPEED, SPEED);
        unsigned long t_blind = millis();
        while(millis() - t_blind < 200) { server.handleClient(); webSocket.loop(); }
      }

      currentIndex++;
      
      // Update lại lastNodeTime SAU KHI đã rẽ hoặc qua ngã tư xong
      lastNodeTime = millis();"""

text = text.replace(orig_loop, new_loop)

with open('/Users/thaongan/Downloads/CarLine/CarLine.ino', 'w') as f:
    f.write(text)

print("FIXED: DEBOUNCE AND BLIND TURN BUG")
