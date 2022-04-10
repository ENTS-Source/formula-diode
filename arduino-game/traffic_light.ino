#define TRAFFIC_START 7
#define TRAFFIC_LENGTH 6

CRGB TRAFFIC_RED = CRGB(255, 0, 0);
CRGB TRAFFIC_YELLOW = CRGB(255, 200, 0);
CRGB TRAFFIC_GREEN = CRGB(0, 255, 0);

void runTraffic() {
  CRGB lights[TRAFFIC_LENGTH] = {
    CRGB::Black,
    CRGB::Black,
    CRGB::Black,
    CRGB::Black,
    CRGB::Black,
    CRGB::Black,
  };
  drawEntity(TRAFFIC_START, lights, TRAFFIC_LENGTH);
  delay(1000);

  lights[0] = TRAFFIC_RED;
  lights[1] = TRAFFIC_RED;
  drawEntity(TRAFFIC_START, lights, TRAFFIC_LENGTH);
  delay(1000);

  lights[2] = TRAFFIC_YELLOW;  
  lights[3] = TRAFFIC_YELLOW;  
  drawEntity(TRAFFIC_START, lights, TRAFFIC_LENGTH);
  delay(1000);

  lights[0] = TRAFFIC_GREEN;
  lights[1] = TRAFFIC_GREEN;
  lights[2] = TRAFFIC_GREEN;  
  lights[3] = TRAFFIC_GREEN;  
  lights[4] = TRAFFIC_GREEN;  
  lights[5] = TRAFFIC_GREEN;  
  drawEntity(TRAFFIC_START, lights, TRAFFIC_LENGTH);
  delay(1000);
}
