/* Project: Comunicacion Servidor - Teclado con visualizacion en dos puertos
 * By: Jacobo Rave and Juan Guerra
 * Version: 1.3
 */

#include <Keypad.h> 
#include <WiFi.h>
#include <LiquidCrystal.h>

const char* ssid = "SSID";
const char* password = "PASSWORD";

WiFiServer server(80);

int limit[10]={1, 3, 3, 3, 3, 3, 3, 3, 3, 3};
int lett[10]={' ', 'A', 'D', 'G', 'J', 'M', 'O', 'R', 'U', 'X'};
char teclas[4][4] ={
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
  }; 

byte pinesfilas[4] = {14,27,26,25};
byte pinescolumnas[4] = {13,22,12,3};  

Keypad teclado = Keypad(makeKeymap(teclas), pinesfilas, pinescolumnas, 4,4);    
LiquidCrystal lcd(19, 23, 18, 17, 16, 15);
WiFiClient client;

int t_inicial, t_actual, pos;
char k, typeTemp;
String msg, msgByServer;
String last_msg;

void setup() {
  pinMode(4, OUTPUT);
  pinMode(4, 0);
  lcd.begin(16, 2);
  
  Serial.begin(115200);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  
}

// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 10000;
void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients 
  
  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            int ind=header.indexOf("GET ?msgByServer=")+19;
            msgByServer="";
            while(header[ind]!=' '){
              Serial.println("Analizando...");
              Serial.println(ind);

              if(header[ind]=='+'){
                msgByServer = msgByServer + " ";
              }else{
                lcd.clear();
                msgByServer = msgByServer + header[ind];  
              }              
              ind++;     
            }
            
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            //client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            String form_html = "";
              form_html += "<form method = 'GET'>";
              form_html += "<label for = 'msgByServer'>Mensaje a enviar </label>";
              form_html += "<input type = 'text' value='' name='msgByServer'/>";
              form_html += "<input type='submit' value='Send' />";
              form_html += "</form>";
            client.println("<body><h1>ESP32 Web Server</h1>");
            //client.println("<p>Escribe un mensaje: <input></ input></p>");
            client.println("<p>Mensaje desde NodeMCU: " + msg+ "</p>");
            client.println(form_html);
            client.println("</body>");
            client.println("<meta http-equiv=\"refresh\" content=\"10\">");
            
            client.println("</html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
  typing();
  lcd.setCursor(0, 0);
  lcd.print(msgByServer);
}

void typing(){
  //Si no hay entrada previa registrada, espera una lectura.
  if(k=='\0'){  
    k = waitRead(1000);
    if(k=='\0'){  
      Serial.println(last_msg + " " + msg);
      last_msg = last_msg == msg ? "-1" : last_msg;
      return;
    }
  }

  //Registro de la tecla presionada y el numero de veces que se ha hecho.
  typeTemp = k;
  pos=0;
  
  //Delete: Eliminar el último caracter.
  if(k == 'D'){
    msg=msg.substring(0, msg.length()-1);
    k='\0';
  }
  
  //Clear: Limpiar el mensaje.
  else if(k == 'C'){
    msg="";
    k='\0';
  }
  
  //Actualizar el mensaje con la entrada que se maneje desde handleInput()
  else{
    handleInput();
    msg = msg + interpret(typeTemp, pos);
    last_msg = msg;
  }
  
  //Visualización del mensaje local
  Serial.print("El mensaje local va así: ");
  Serial.println(msg);
  }

char interpret(int typeTemp, int pos){ 
  if((typeTemp-'0')==5 && pos==2){
    return 'Ñ';
  }  
  char l = lett[typeTemp-'0'] + (pos);
  return l;
}

void handleInput(){
  while(true){
    k=waitRead(1000);
    if(k=='\0' || k!=typeTemp || pos==limit[k-'0']){
      return;
    }
    pos++;
  }    
}
  
char waitRead(int tiempo){
  t_inicial=millis();
  t_actual=millis();

  k=teclado.getKey();
  while((k=='\0' || k=='A' || k=='B' || k=='#' || k=='*') && (t_actual-t_inicial)<=tiempo){
    k=teclado.getKey();
    t_actual=millis();
  }
  Serial.println(k);
  return k;
}

char waitRead(){
  k=teclado.getKey();
  while(k=='\0' || k=='A' || k=='B' || k=='#' || k=='*'){
    k=teclado.getKey();
  }
  Serial.println(k);
  return k;
}
