#include <esp_now.h>
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ezButton.h>

#define OLED_RESET 16
Adafruit_SSD1306 display(OLED_RESET);

//id
int id;

//buttons
ezButton button1(4);
ezButton button2(5);

//player MAC
uint8_t player1Address[] = {0x3C,0x71,0xBF,0x10,0x5C,0x3C};
uint8_t player2Address[] = {0x3C,0x61,0x05,0x04,0x3F,0x18};

//creditor MAC
uint8_t creditor1Address[] = {0xA4, 0xCF, 0x12, 0x8F, 0xBA, 0x18};

int coinCount;

esp_now_peer_info_t peerInfo;

typedef struct game_state_message {
  int state;
  int id;
  char message[32];
  bool is_ready;
  int bet_amount;
  bool hit;
  int WithdrawCredit;
} game_state_message;

typedef struct dealer_message {
  char message[32]; //message
  int player_state;  //state number 0 = init , 1=bet , 2 = playing, 3 = waiting
  int player1_result; //result number 0 = lose, 1 = win, 2 = draw
  int player2_result; //result number 0 = lose, 1 = win, 2 = draw
  int player1_card[5];
  int player2_card[5];
  int dealer_card[5];
  int FromWho; //if 0 = Dealer, 1 = Coin master 
  int DepositCredit;
} dealer_message;

game_state_message gameStateMessage;
dealer_message dealerMessage;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&gameStateMessage, incomingData, sizeof(gameStateMessage));

  Serial.println("Data Recieved!");
}

void SendCoinSignal() {
  coinCount++;
  dealerMessage.FromWho = 1;
  dealerMessage.DepositCredit = 100;

  if (id == 1) {
    esp_err_t result1 = esp_now_send(player1Address, (uint8_t *) &dealerMessage, sizeof(dealerMessage));
    while(result1 != ESP_OK)
    {
      result1 = esp_now_send(player1Address, (uint8_t *) &dealerMessage, sizeof(dealerMessage));
    }
    return;
  } else {
    esp_err_t result1 = esp_now_send(player2Address, (uint8_t *) &dealerMessage, sizeof(dealerMessage));
    while(result1 != ESP_OK)
    {
      result1 = esp_now_send(player2Address, (uint8_t *) &dealerMessage, sizeof(dealerMessage));
    }
    return;
  }
}

void CoinMachineDisplay() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(BLACK, WHITE);
  display.setCursor(0,0);
  display.print("Coin : ");
  display.println(coinCount);
  display.display();
}

void setup() {
  // put your setup code here, to run once:
  button1.setDebounceTime(300); 
  button2.setDebounceTime(300);
  coinCount = 0;

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c); //สั่งให้จอ OLED เริ่มทำงานที่ Address 0x3C
  display.clearDisplay();

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializeing ESP-NOW");
    return;
  }

  //Register send callback
  esp_now_register_send_cb(OnDataSent);
  
  //Register receive callback
  esp_now_register_recv_cb(OnDataRecv);

  uint8_t myMacAddress[6];
  WiFi.macAddress(myMacAddress);

  if (memcmp(myMacAddress, creditor1Address, 6) == 0) {
    Serial.println("This is creditor 1");
    id = 1;
  } else {
    Serial.println("This is creditor 2");
    id = 2;
  }

  //Register peer
  memset(peerInfo.peer_addr, 0, sizeof(peerInfo));

  if (id == 1) {
    for(int i=0; i<6; ++i) {
      peerInfo.peer_addr[i] = (uint8_t) player1Address[i];
    }
  } else {
    for(int i=0; i<6; ++i) {
      peerInfo.peer_addr[i] = (uint8_t) player2Address[i];
    }
  }
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  //Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Fail to add peer");
    return;
  }
}

void handleCoinInsert();
void handleCoinWithdraw();

void loop() {
  // put your main code here, to run repeatedly:
  button1.loop();
  button2.loop();

  CoinMachineDisplay();
  handleCoinInsert();
  handleCoinWithdraw();
}

void handleCoinInsert() {
  if(button1.isPressed()){ //if hit
    SendCoinSignal();
  }
}

void handleCoinWithdraw() {

}