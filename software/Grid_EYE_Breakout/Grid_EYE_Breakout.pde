import processing.serial.*;

Serial uart;
int theKey;
String theBuf;
int theSize = 8;
int theScale = 128;
float[][] theBitmap = new float[8][8];
float theTemp = 27.0;
float theAdj = 75;
float min = 0xffff;
float max = 0;
float ave = 0;
int max_x;
int max_y;

int state = 0;

void setup() {
  size(theSize*theScale, theSize*theScale);
  textSize(24);
  
  print(Serial.list());
  String uartName = Serial.list()[1];   // <-- may have to modiy this depending on the com port of the board 
  print("\n\rSelected:" + uartName);
  uart = new Serial(this, uartName, 115200);
  uart.bufferUntil('\n');
}

void draw() {

  min = 0xffff;
  max = 0;

  
  ave = 0;
  for (int i=0; i<theSize; i++) {
    for (int j=0; j<theSize; j++) {
      ave += theBitmap[i][j];
      if (theBitmap[i][j] < min)
      {
        min = theBitmap[i][j];
      }
      if (theBitmap[i][j] > max)
      {
        max = theBitmap[i][j];
        max_x = i;
        max_y = j;
      }
    }
  }
  ave /= (8*8);

  background(0);
  for (int i=0; i<theSize; i++) {
    for (int j=0; j<theSize; j++) {
      float temp = theBitmap[i][j];
      float r = 0;
      float b = 0;
      float g = 0;

      g = map(temp, min, max, 0, 75);

      if (temp > ave) {
        r = map(temp, min, max, 0, 255);
      } else if (temp < ave) {
        b = map(temp, min, max, 0, 255);
      }

      stroke(r, g, b);
      fill(r, g, b);
      rect(i*theScale, (theSize -1  - j)*theScale, theScale, theScale);
      stroke(255);
      fill(255);

      // uncomment this line for celcius display
      //text(int(temp), (i+0.1)*theScale,(j+0.5)*theScale);
      text(int(temp), (i+0.1)*theScale, ((theSize -1  - j)+0.5)*theScale);
      // uncomment this line for farenheit display
      //      text(int(1.8*temp+32), (i+0.1)*theScale,(j+0.85)*theScale);
      
      if((i == max_x) && (j==max_y))
      {
      stroke(255, 255, 0);
      fill(255, 255, 0);
      rect(i*theScale, (theSize -1  - j)*theScale, theScale, theScale);
      stroke(0);
      fill(0);
      text(int(temp), (i+0.1)*theScale, ((theSize -1  - j)+0.5)*theScale);
      }
    }
  }
}

void serialEvent(Serial port) {
  theBuf = trim(port.readString());
    println(theBuf);
  if (theBuf.length() == 9) {
    if (theBuf.equals("Grid-EYE:")) state = 1;
  } else if (state > 0 && state <= 8) {
    float[] pix = float(split(theBuf, ' '));
    for (int i=0; i<theSize; i++) {
      theBitmap[i][state-1] = pix[i];
      //      theBitmap[i][state-1] = (pix[i]-20)*16;
    }
    state++;
  } else if (state == 9) {
    // get ambient temp
    //    theTemp = float(theBuf);
    // println(pix[i]);
    state = 0;
  }
}

void keyPressed() {
  uart.write(key);
  theKey = key;
}

