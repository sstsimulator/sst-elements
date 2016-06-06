import sst
from sst.pyproto import *


sst.setProgramOption("stopAtCycle", "10s")


class MyObject(PyProto):
    def __init__(self, name):
        PyProto.__init__(self, name)
        self.name = name
        self.countdown = 10

    def setActiveLink(self, link):
        self.myActiveLink = self.addLink(link, "1us", self._linkHandle)

    def _linkHandle(self, event):
        print self.name, "LinkHandle %s"%event.type
        print self.name, "LinkHandle: ", event.sst


    def construct(self):
        print self.name, "Construct()"

    def init(self, phase):
        print self.name, "init(%d)"%phase

    def setup(self):
        print self.name, "setup()"

    def finish(self):
        print self.name, "finish()"



link0 = sst.Link("Mylink0")

comp_msgGen0 = sst.Component("msgGen0", "simpleElementExample.simpleMessageGeneratorComponent")
comp_msgGen0.addParams({
      "outputinfo" : 1,
      "sendcount" : 1,
      "clock" : "1MHz"
})
comp_msgGen0.addLink(link0, "remoteComponent", "10us")

alice = MyObject("Alice")
alice.setActiveLink(link0)



