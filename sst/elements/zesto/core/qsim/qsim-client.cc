#ifdef USE_QSIM

#include <qsim.h>
#include <iostream>
#include <vector>

#include "qsim-net.h"
#include "qsim-client.h"

void Qsim::ClientQueue::inst_cb
  (int c, uint64_t va, uint64_t pa, uint8_t l, const uint8_t *b)
{
  if (cpu == c) this->push(Qsim::QueueItem(va, pa, l, b));
}

void Qsim::ClientQueue::mem_cb
(int c, uint64_t va, uint64_t pa, uint8_t s, int t)
{
  if (cpu == c) this->push(Qsim::QueueItem(va, pa, s, t));
}

int Qsim::ClientQueue::int_cb(int c, uint8_t v) {
  if (cpu == c) this->push(Qsim::QueueItem(v));
  return 0;
}

void Qsim::Client::process_callbacks() {
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
                uint8_t len, *bytes(inst_bytes), type_b;
                sbs >> i >> vaddr >> paddr >> len >> type_b;
                enum inst_type type((enum inst_type)type_b);
                QsimNet::recvdata(sbs.fd, (char *)bytes, len);
                for (unsigned I = 0; I < inst_cbs.size(); I++)
                  (*inst_cbs[I])(i, vaddr, paddr, len, inst_bytes, type);
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
                uint8_t size, type;
                uint32_t val;
                sbs >> i >> port >> size >> type >> val;
                for (unsigned I = 0; I < io_cbs.size(); I++)
                  (*io_cbs[I])(i, port, size, type, val);
              }
              break;
    case 'r': { uint16_t i;
                uint32_t reg;
                uint8_t size, type;
                sbs >> i >> reg >> size >> type;
                for (unsigned I = 0; I < reg_cbs.size(); I++)
                  (*reg_cbs[I])(i, reg, size, type);
              }
              break;
    case 'u': { char c;
                uint16_t i;
                sbs >> c >> i;
                switch (c) {
                case 'T': { uint16_t tid;
                            sbs >> tid;
                            tid_map[i] = tid;
                          }
                          break;
                case 'M': { int8_t mode;
                            sbs >> mode;
                            mode_map[i] = Qsim::OSDomain::cpu_mode(mode);
                          }
                          break;
                case 'P': { int8_t prot;
                            sbs >> prot;
                            prot_map[i] = Qsim::OSDomain::cpu_prot(prot);
                          }
                          break;
                case 'I': { int8_t idle;
                            sbs >> idle;
                            idle_map[i] = (idle != 0);
                          }
                          break;
                }
              }
              break;
    case '.': return;
    default: { std::cout << "Callback format error.\n";
               exit(1);
             }
    }
  }
}


#endif // USE_QSIM
