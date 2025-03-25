#include <filesystem>
#include <iostream>
#include <fstream>

#include "frontend/frontend.h"
#include "base/exception.h"

namespace Ramulator {


class SST : public IFrontEnd, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(IFrontEnd, SST, "SST", "SST frontend.")

  public:
    void init() override {};

    void tick() override {
      m_memory_system->tick();
    };

    bool receive_external_requests(int req_type_id, Addr_t addr, int source_id, std::function<void(Request&)> callback) override {
      return m_memory_system->send({addr, req_type_id, source_id, callback});
    }

  private:
    bool is_finished() override { return true; };
};

}        // namespace Ramulator