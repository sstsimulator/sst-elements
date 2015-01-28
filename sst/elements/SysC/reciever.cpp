#include "reciever.h"
#include <iostream>
using std::cout;
using std::endl;

void Reciever::processInput(){
  cout<<"sc_in_ = "<<sc_in_.read()<<endl;
  cout<<"native_in_ = "<<native_in_.read()<<endl;
}
