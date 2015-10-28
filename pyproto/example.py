import sst
from sst.pyproto import *


class MyEvent(PyEvent):
    def __init__(self, cycle, count):
        PyEvent.__init__(self)
        self.cycle = cycle
        self.count = count

    def __str__(self):
        return "{%d, %d}"%(self.cycle, self.count)


class MyObject(PyProto):
    def __init__(self, name):
        PyProto.__init__(self, name)
        self.name = name
        self.countdown = 10

        self.addClock(self._clockHandle, "1MHz")

    def setPollLink(self, link):
        self.myPollLink = self.addLink(link, "1us")

    def setActiveLink(self, link):
        self.myActiveLink = self.addLink(link, "1us", self._linkHandle)

    def _linkHandle(self, event):
        print self.name, "LinkHandle ", event

    def _clockHandle(self, cycle):
        print self.name, "Clock ", cycle
        ev = self.myPollLink.recv()
        if ev:
            print self.name, "Received on pollLink: ", ev
        if 0 == (cycle % 10):
            ev = MyEvent(cycle, self.countdown)
            print self.name, "Sending event", ev
            link = self.myPollLink if 0 == (self.countdown%2) else self.myActiveLink
            link.send(ev)
            self.countdown -= 1
        return (self.countdown == 0)

    def construct(self):
        print self.name, "Construct()"

    def init(self, phase):
        print self.name, "init(%d)"%phase

    def setup(self):
        print self.name, "setup()"

    def finish(self):
        print self.name, "finish()"



link0 = sst.Link("Mylink0")
link1 = sst.Link("Mylink1")


alice = MyObject("Alice")
alice.setPollLink(link0)
alice.setActiveLink(link1)

bob = MyObject("Bob")
bob.setPollLink(link0)
bob.setActiveLink(link1)


