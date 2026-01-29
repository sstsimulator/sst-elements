void requireLibraryForward(std::string library) override {
  requireLibrary(library);
}

Link *selfEventLink() override {
  return selfEventLink_;
}

void handleEvent(SST::Event *ev) override {
  OperatingSystemImpl::handleEvent(ev);
}

std::function<void(NetworkMessage *)> nicDataIoctl() override {
  return OperatingSystemImpl::nicDataIoctl();
}

std::function<void(NetworkMessage *)> nicCtrlIoctl() override {
  return OperatingSystemImpl::nicCtrlIoctl();
}

void setParentNode(NodeBase *parent) override {
  OperatingSystemImpl::setParentNode(parent);
}

NodeBase *node() const override {
  return OperatingSystemImpl::node();
}

NodeId addr() const override {
  return OperatingSystemImpl::addr();
}

void setNumRanks(unsigned int ranks) override {
  OperatingSystemImpl::setNumRanks(ranks);
}

unsigned int numRanks() override {
  return OperatingSystemImpl::numRanks();
}

void setRanksPerNode(unsigned int npernode) override {
  OperatingSystemImpl::setRanksPerNode(npernode);
}

unsigned int ranksPerNode() override {
  return OperatingSystemImpl::ranksPerNode();
}

void startApp(App *theapp, const std::string &unique_name) override {
  OperatingSystemImpl::startApp(theapp, unique_name);
}

Thread *activeThread() const override {
  return OperatingSystemImpl::activeThread();
}

void startThread(Thread *t) override {
  OperatingSystemImpl::startThread(t);
}

void joinThread(Thread *t) override {
  OperatingSystemImpl::joinThread(t);
}

void switchToThread(Thread *tothread) override {
  OperatingSystemImpl::switchToThread(tothread);
}

void scheduleThreadDeletion(Thread *thr) override {
  OperatingSystemImpl::scheduleThreadDeletion(thr);
}

void completeActiveThread() override {
  OperatingSystemImpl::completeActiveThread();
}

void block() override {
  OperatingSystemImpl::block();
}

void unblock(Thread* thr) override {
  OperatingSystemImpl::unblock(thr);
}

void blockTimeout(TimeDelta delay) override {
  OperatingSystemImpl::blockTimeout(delay);
}

UniqueEventId allocateUniqueId() override {
  return OperatingSystemImpl::allocateUniqueId();
}

int allocateMutex() override {
  return OperatingSystemImpl::allocateMutex();
}

mutex_t *getMutex(int id) override {
  return OperatingSystemImpl::getMutex(id);
}

bool eraseMutex(int id) override {
  return OperatingSystemImpl::eraseMutex(id);
}

int allocateCondition() override {
  return OperatingSystemImpl::allocateCondition();
}

condition_t *getCondition(int id) override {
  return OperatingSystemImpl::getCondition(id);
}

bool eraseCondition(int id) override {
  return OperatingSystemImpl::eraseCondition(id);
}

Library *eventLibrary(const std::string &name) const override {
  return OperatingSystemImpl::eventLibrary(name);
}

void registerEventLib(Library *lib) override {
  OperatingSystemImpl::registerEventLib(lib);
}

void unregisterEventLib(Library *lib) override {
  OperatingSystemImpl::unregisterEventLib(lib);
}

void handleRequest(Request *req) override {
  OperatingSystemImpl::handleRequest(req);
}
