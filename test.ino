#include "NeuralEngine.h"

void setup() {
  Serial.begin(115200);
  delay(2000);
  // Primitive Data Types 
//   Serial.println("Serial OK!");

//   char name[20];
//   char letter;
//   int age;
//   float height;
//   double weight;

// Serial.print("Enter Name: ");
//   readString(name, sizeof(name));
//   Serial.print("You typed: ");
//   Serial.println(name);

//   Serial.print("Enter a Char: ");
//   letter = readChar();
//   Serial.print("Char: ");
//   Serial.println(letter);

//   Serial.print("Enter Age: ");
//   age = readInt();
//   Serial.print("Age: ");
//   Serial.println(age);

//   Serial.print("Enter Height: ");
//   height = readFloat();
//   Serial.print("Height: ");
//   Serial.println(height);

//   Serial.print("Enter Weight: ");
//   weight = readDouble();
//   Serial.print("Weight: ");
//   Serial.println(weight);

  // Non Primitive Data Types
//   int a[2][2]={{1,2},{1,2}};
//   cl("%d",*(*(a+1)+1));

//   for (int i=0;i<2;i++){
//       cl("%d",a[i][j]);
//   }
  
  
  struct node{
    int a;
    float b;
    char c;
  }; 

  struct node n1;
  n1.a=20;
  n1.b=32.3;
  n1.c='A';
 cl("%d,%f,%c",n1.a,n1.b,n1.c);

 char buf[32];

 clearEEP(); // ← reset memory first

  set_item("name", "Kishore");
  set_item("city", "Salem");

  char* n = get_item_simple("name");
  cl(n);

  char* c = get_item_simple("city");
  cl(c);
}
void loop(){
  // Serial.println(stc("hiii"));
  
}

