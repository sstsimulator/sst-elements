#include <qsim.h>
#include <iostream>
#include <vector>

#include "zesto-qsim.h"

// TODO: Migrate the stuff below to a header file/library

void Queue::inst_cb
  (int c, uint64_t va, uint64_t pa, uint8_t l, const uint8_t *b)
{
  if (cpu == c) this->push(Qsim::QueueItem(va, pa, l, b));
}

void Queue::mem_cb
(int c, uint64_t va, uint64_t pa, uint8_t s, int t)
{
  if (cpu == c) this->push(Qsim::QueueItem(va, pa, s, t));
}

int Queue::int_cb(int c, uint8_t v) {
  if (cpu == c) this->push(Qsim::QueueItem(v));
  return 0;
}

void QsimClient::process_callbacks() {
  while (1) {
    char c;
    sbs >> c;
    switch (c) {
    case 'a': { uint16_t i;
                sbs >> i;
                char response = 'F';
                for (unsigned I = 0; I < atomic_cbs.size(); I++)
                  if ((*atomic_cbs[I])(i)) response = 'T';
                sbs << response;
              }
              break;
    case 'i': { uint16_t i;
                uint64_t vaddr, paddr; 
                uint8_t len;
                uint8_t *bytes = inst_bytes;
                sbs >> i >> vaddr >> paddr >> len;
                QsimNet::recvdata(sbs.fd, (char *)bytes, len);
                for (unsigned I = 0; I < inst_cbs.size(); I++)
                  (*inst_cbs[I])(i, vaddr, paddr, len, inst_bytes);
              }
              break;
    case 'v': { uint16_t i;
                uint8_t vec;
                sbs >> i >> vec;
                char response = 'F';
                for (unsigned I = 0; I < inst_cbs.size(); I++)
                  if ((*int_cbs[I])(i, vec)) response = 'T';
                sbs << response;
              }
              break;
    case 'm': { uint16_t i;
                uint64_t vaddr, paddr;
                uint8_t size, type;
                sbs >> i >> vaddr >> paddr >> size >> type;
                for (unsigned I = 0; I < mem_cbs.size(); I++)
                  (*mem_cbs[I])(i, vaddr, paddr, size, type);
              }
              break;
    case 'g': { uint16_t i;
                uint64_t rax;
                sbs >> i >> rax;
                char response = 'F';
                for (unsigned I = 0; I < magic_cbs.size(); I++)
                  if ((*magic_cbs[I])(i, rax)) response = 'T';
                sbs << response;
              }
              break;
    case 'o': { uint16_t i;
                uint64_t port;
                uint8_t size;
                uint8_t type;
                uint32_t val;
                sbs >> i >> port >> size >> type >> val;
                for (unsigned I = 0; I < io_cbs.size(); I++)
                  (*io_cbs[I])(i, port, size, type, val);
              }
              break;
    case '.': return;
    default: { std::cout << "Callback format error.\n";
               exit(1);
             }
    }
  }
}

// TODO: Migrate the stuff above to a header file/library

/*
struct CbTester {
  int atomic(int cpu) {
    return 1;
  }
  void inst(int cpu, uint64_t va, uint64_t pa, uint8_t l, const uint8_t *b) {
    static unsigned count = 0;
    if (count++ == 1000000) count = 0; else return;
    cout << "Instruction, CPU " << cpu << " va=0x" << std::hex << va << ": ";
    for (unsigned i = 0; i < l; i++) 
      cout << std::hex << std::setw(0) << std::setfill('0') << (b[i]&0xff) 
           << ' ';
    cout << '\n';
  }
  void mem(int cpu, uint64_t va, uint64_t pa, uint8_t s, int t) {
    static unsigned count = 0; 
    if (count++ == 1000000) count = 0; else return;
    cout << "Memory op, CPU " << cpu << " va=0x" << std::hex << va << " size="
         << (s&0xff) << ' ' << (t?'W':'R') << '\n';
  }
} cbtest;

int main(int argc, char **argv) {
  if (argc != 3) {
    cout << "Usage: " << argv[0] << " <server> <port>\n";
    return 1;
  }

  QsimClient *qc = new QsimClient(client_socket(argv[1], argv[2]));

  qc->set_atomic_cb(&cbtest, &CbTester::atomic);
  qc->set_inst_cb(&cbtest, &CbTester::inst);
  qc->set_mem_cb (&cbtest, &CbTester::mem );

  cout << "Server has " << qc->get_n() << " CPUs.\n";
  for (unsigned i = 0; i < qc->get_n(); i++) {
    cout << "  CPU " << i << " is " << (qc->booted(i)?"booted.\n":"offline.\n");
  }

  unsigned *inst_countdown = new unsigned[sizeof(unsigned)*qc->get_n()];

  for (unsigned i = 0; i < 10000; i++) {
    for (unsigned j = 0; j < qc->get_n(); j++)
      inst_countdown[j] = (qc->booted(j)?10000000:0);

    bool running;
    do {
      for (unsigned j = 0; j < qc->get_n(); j++)
        inst_countdown[j] -= qc->run(j, inst_countdown[j]);
    
      running = false;
      for (unsigned j = 0; j < qc->get_n(); j++) 
        if (inst_countdown[j] != 0) running = true;
    } while(running);

    qc->timer_interrupt();
  }

  delete inst_countdown;
  delete qc;

  return 0;
}*/
