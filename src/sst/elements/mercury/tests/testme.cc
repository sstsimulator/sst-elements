#define ssthg_app_name testme
#include <libraries/system/replacements/unistd.h>
#include <iostream>
#include <common/skeleton.h>
int main(int argc, char** argv) {
  std::cerr << "Hello from Mercury!\n";
  std::cerr << "Now I will sleep\n";
  sleep(5);
  std::cerr << "I'm back!\n";
  std::cerr << "Bye!\n"; 
  return 0;
}
