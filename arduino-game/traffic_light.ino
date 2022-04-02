#define TRAFFIC_START 5

CRGB TRAFFIC_YELLOW = CRGB(255, 200, 0);
CRGB TRAFFIC_GREEN = CRGB(0, 255, 0);

void runTraffic() {
  CRGB lights[4] = {
    CRGB::Black,
    CRGB::Black,
    CRGB::Black,
    CRGB::Black,
  };
  drawEntity(TRAFFIC_START, lights, 4);
  delay(1000);

  lights[0] = TRAFFIC_YELLOW;  
  drawEntity(TRAFFIC_START, lights, 4);
  delay(1000);

  lights[1] = TRAFFIC_YELLOW;  
  drawEntity(TRAFFIC_START, lights, 4);
  delay(1000);

  lights[2] = TRAFFIC_YELLOW;  
  drawEntity(TRAFFIC_START, lights, 4);
  delay(1000);

  lights[3] = TRAFFIC_GREEN;  
  drawEntity(TRAFFIC_START, lights, 4);
  delay(1000);
}
