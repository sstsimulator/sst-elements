"""Resident byte bounds, nonempty footprints, partial banks, and aligned decode."""
import sst

guard = sst.Component("guard", "carcosa.EccFaultModelTest")
guard.addParams({
    "state_key": "",
    "ecc_scheme": "secded",
    "fault_model": "resident",
    "resident_addr_start": 0x4000,
    "resident_addr_len": 64,
    "resident_fault_rate_per_ms": 0,
    "resident_faults_at_start": 0,
    "seed": 12345,
})
sst.Link("loopback").connect((guard, "highlink", "1ns"), (guard, "lowlink", "1ns"))
