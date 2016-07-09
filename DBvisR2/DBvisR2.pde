/*-----------
Dual Brains Data Viz - R2
This sketch should
1. initialized necessary Graph objects & a UDP object
2. Parse data read via UDP object and pipe the correct data into the Graph objects
------------- */
LineGraph g;
float[][] data_list;
void setup(){
  size(1200,750);

  //Test Graph
  //Graph(float SAMPLE_RATE, int TIME_WINDOW, float SCALE, int ORIGIN_X, int ORIGIN_Y){
  //LineGraph(int CHANNELS, float UPPER_LIM, float LOWER_LIM, float SAMPLE_RATE, int TIME_WINDOW, float SCALE, float ORIGIN_X, float ORIGIN_Y){
  g = new LineGraph(6, 250, -250, 20,5, 6, 20, height-20);
}

void draw(){
  background(0);
  // colorMode(RGB, 255,255,255,100);
  // fill(color(0,0,0,10));
  // rect(0,0,width,height);
  float[] newData = {random(-250,250), random(-250,250), random(-250,250), random(-250,250), random(-250,250), random(-250,250)};
  g.update(newData);
  g.render();
  text(frameRate, 20, 20);
}

void mousePressed(){
  g.debugMode = !g.debugMode;
}