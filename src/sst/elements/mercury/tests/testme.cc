#define ssthg_app_name testme
#include <libraries/system/replacements/unistd.h>
#include <iostream>
#include <common/skeleton.h>
int main(int argc, char** argv) {
  std::cout << "Hello from Mercury!\n";
  std::cout << "Now I will sleep\n";
  sleep(5);
  std::cout << "I'm back!\n";
  std::cout << "Bye!\n"; 
  return 0;
}
