"""A posted control write must update the agent without returning an ack."""
import sst
from haliEdgeCommon import build

sst.setProgramOption("stop-at", "1us")
build("posted_write")
