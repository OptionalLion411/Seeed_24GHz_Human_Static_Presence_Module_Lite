#include "humanstaticLite.h"

HumanStaticLite::HumanStaticLite(Stream *s) : stream(s) {}

void HumanStaticLite::recvData() {
  while (stream->available()) {
    if (stream->read() == MESSAGE_HEAD1) {           //Receive header frame 1
      if (stream->read() == MESSAGE_HEAD2) {         //Receive header frame 2
        dataLen = stream->readBytesUntil(MESSAGE_END2, Msg, 20);
        if (dataLen > 0 && dataLen < 20){
          Msg[dataLen - 1] = MESSAGE_END1;
          Msg[dataLen] = MESSAGE_END2;
          this->newData = true;
        }
      }
    }
  }
}

void HumanStaticLite::parseData(bool bodysign) {
  radarStatus = 0x00;
  bodysign_val = 0;
  static_val = 0;
  dynamic_val = 0;
  dist_static = 0;
  dist_move = 0;
  speed = 0;
  if(this->newData){
    switch (Msg[0]) {
      case HUMANSTATUS:
        switch (Msg[1]) {
          case HUMANEXIST:
            switch (Msg[4]) {
              case SOMEBODY:
                radarStatus = SOMEONE;
                break;
              case NOBODY:
                radarStatus = NOONE;
                break;
            }
            break;
          case HUMANMOVE:
            switch (Msg[4]) {
              case NONE:
                radarStatus = NOTHING;
                break;
              case SOMEBODY_STOP:
                radarStatus = SOMEONE_STOP;
                break;
              case SOMEBODY_MOVE:
                radarStatus = SOMEONE_MOVE;
                break;
            }
            break;
          case HUMANSIGN:
            if (bodysign) {
              radarStatus = HUMANPARA;
              bodysign_val = Msg[4];
            }
            break;
          case HUMANDIRECT:
            switch (Msg[4]) {
              case NONE:
                radarStatus = NOTHING;
                break;
              case CA_CLOSE:
                radarStatus = SOMEONE_CLOSE;
                break;
              case CA_AWAY:
                radarStatus = SOMEONE_AWAY;
                break;
            }
            break;
        }
        break;
      case DETAILSTATUS:
        switch (Msg[1]) {
          case DETAILINFO:
            radarStatus = DETAILMESSAGE;
            static_val = Msg[4];
            dist_static = decodeVal(Msg[5]);
            dynamic_val = Msg[6];
            dist_move = decodeVal(Msg[7]);
            speed = decodeSpeed(Msg[8]);
            break;
          case DETAILDIRECT:
            switch(Msg[4]){
              case NONE:
                radarStatus = NOTHING;
                break;
              case CA_CLOSE:
                radarStatus = SOMEONE_CLOSE;
                break;
              case CA_AWAY:
                radarStatus = SOMEONE_AWAY;
                break;
            }
            break;
          case DETAILSIGN:
            if(bodysign){
              radarStatus = HUMANPARA;
              bodysign_val = Msg[4];
            }
            break;
        }
        break;
    }
  }
  this->newData = false;
}

void HumanStaticLite::fetchFrame(bool bodysign) {
  recvData();
  parseData(bodysign);
}

void HumanStaticLite::setMode(const unsigned char* buff, int len, bool cyclic) {
  if (cyclic || count < checkdata_len) {
    if (cyclic || count < 1) {
      stream->write(buff, len);
      stream->flush();
    }
    do {
      recvData();
      delay(20);
    } while(!(this->newData));
    //    if (cyclic || count < 1) {
    //      Serial.print("  Sent  ---> ");
    //      data_printf(buff, len);
    //    }
    //    if (count%2 == 1){
    //      Serial.print("Receive <--- ");
    //      showData();
    //    }
    this->newData = false;
  }
  count++;
}

void HumanStaticLite::reset(){
  stream->write(reset_frame, reset_frame_len);
  stream->flush();
}

void HumanStaticLite::showData() {
  if (this->newData){
    Serial.print(MESSAGE_HEAD1, HEX);
    Serial.print(' ');
    Serial.print(MESSAGE_HEAD2, HEX);
    Serial.print(' ');
    char charVal[4];
    for (byte n = 0; n < dataLen+1; n++) {
      sprintf(charVal, "%02X", Msg[n]);
      Serial.print(charVal);
      Serial.print(' ');
    }
    Serial.println();
    this->newData = false;
    Msg[dataLen] = {0};
  }
}


void HumanStaticLite::dPrintf(const unsigned char* buff, int len) {
  char charVal[4];
  for (int i=0; i < len; i++){
    sprintf(charVal, "%02X", buff[i]);
    Serial.print(charVal);
    Serial.print(' ');
  }
  Serial.println();
}

float HumanStaticLite::decodeVal(int val) {
  return val * SEEED_HUMAN_UNIT;
}

float HumanStaticLite::decodeSpeed(int val) {
  if (val == 0x0A) return 0;
  return decodeVal(val > 0x0A ? -(val-10) : val);
}
