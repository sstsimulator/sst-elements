#!/bin/bash

N=${N:=1}
APP=${APP:=$HOME/sst-simulator/qsim/benchmarks/splash2-tar/fft.tar}
STATE=${STATE:=$HOME/qsim/state.$N}
L1LAT=100ps
IPILAT=10ns
BUSLAT=10ns
NETLAT=5ns

CACHEBLOCK=${CACHEBLOCK:=64}
L1WAYS=${L1WAYS:=4}
L1SETS=${L1SETS:=16}
L1ATIME=${L1ATIME:=0ps}

L2WAYS=${L2WAYS:=16}
L2SETS=${L2SETS:=32}
L2ATIME=${L2ATIME:=10ns}

RTR_LINK_BW=${RTR_LINK_BW:=5GHz}
RTR_XBAR_BW=${RTR_XBAR_BW:=5GHz}
NET_BW=${NET_BW:=5GHz}

DRAM_ATIME=${DRAM_ATIME:=50ns}
DRAM_CLOCK=${DRAM_CLOCK:=1GHz}

print_qsim_cpu() {
  j=`expr \( $i + 1 \) % $N`
  h=`expr \( $i + $N - 1 \) % $N`

  echo "  <component name=\"cpu$i\" rank=\"$i\" type=\"qsimComponent\">"
  echo "    <params>"
  echo "      <state>$STATE</state>"
  echo "      <app>$APP</app>"
  echo "      <hwthread>$i</hwthread>"
  echo "    </params>"
  echo "    <link name=c${i}_l1cache_link port=memLink latency=$L1LAT />"
  echo "    <link name=ipi_ring_${i}_$j port=ipiRingOut latency=$IPILAT />"
  echo "    <link name=ipi_ring_${h}_$i port=ipiRingIn latency=$IPILAT />"
  echo "  </component>"
  echo
}

print_trivial_cpu() {
  echo "  <component name=\"cpu$i\" rank=\"$i\" type=\"memHierarchy.trivialCPU\">"
  echo "    <params>"
  echo "      <workPerCycle>1000</workPerCycle>"
  echo "      <commFreq>100</commFreq>"
  echo "      <memSize>0x8000000</memSize>"
  echo "      <doWrite>1</doWrite>"
  echo "    </params>"
  echo "    <link name=c$i_l1cache_link port=memLink latency=$L1LAT />"
  echo "  </component>"
  echo
}

print_l1_cache() {
  echo "  <component name=\"c${i}.l1cache\" rank=\"$i\" type=\"memHierarchy.Cache\">"
  echo "    <params>"
  echo "      <num_ways>$L1WAYS</num_ways>"
  echo "      <num_rows>$L1SETS</num_rows>"
  echo "      <blocksize>$CACHEBLOCK</blocksize>"
  echo "      <access_time>$L1ATIME</access_time>"
  echo "      <num_upstream>1</num_upstream>"
  echo "      <next_level>c${i}.l2cache</next_level>"
  echo "      <printStats>1</printStats>"
  echo "    </params>"
  echo "    <link name=c${i}_l1cache_link port=upstream0 latency=$L1LAT />"
  echo "    <link name=c${i}_l1l2cache_link port=downstream latency=$BUSLAT />"
  echo "  </component>"
  echo
}

print_l2_cache() {
  j=`expr $i + 1`

  echo "  <component name=\"c${i}.l2cache\" rank=\"$i\" type=\"memHierarchy.Cache\">"
  echo "    <params>"
  echo "      <num_ways>$L2WAYS</num_ways>"
  echo "      <num_rows>$L2SETS</num_rows>"
  echo "      <blocksize>$CACHEBLOCK</blocksize>"
  echo "      <access_time>$L2ATIME</access_time>"
  echo "      <num_upstream>1</num_upstream>"
  echo "      <net_addr>$j</net_addr>"
  echo "      <mode>INCLUSIVE</mode>"
  echo "      <printStats>1</printStats>"
  echo "    </params>"
  echo "    <link name=c${i}_l1l2cache_link port=upstream0 latency=$L1LAT />"
  echo "    <link name=cache_net_${i} port=directory_link latency=$BUSLAT />"
  echo "  </component>"
  echo
}

print_network() {
  O=`expr $N + 1`

  echo "  <component name=\"chiprtr\" rank=\"0\" type=\"merlin.hr_router\">"
  echo "    <params>"
  echo "      <num_ports>$O</num_ports>"
  echo "      <num_vcs>3</num_vcs>"
  echo "      <link_bw>$RTR_LINK_BW</link_bw>"
  echo "      <xbar_bw>$RTR_XBAR_BW</xbar_bw>"
  echo "      <topology>merlin.singlerouter</topology>"
  echo "      <id>0</id>"
  echo "    </params>"
  echo "    <link name=dir_net_0 port=port0 latency=$NETLAT />"

  for j in `seq 1 $N`; do
    h=`expr $j - 1`
    echo "    <link name=cache_net_$h port=port$j latency=$NETLAT />"
  done

  echo "  </component>"
  echo
}

print_dirmem() {
  echo -n "  <component name=\"dirctrl0\" rank=\"0\""
  echo       "type=\"memHierarchy.DirectoryController\">"
  echo "    <params>"
  echo "      <network_addr>0</network_addr>"
  echo "      <network_bw>$NET_BW</network_bw>"
  echo "      <addrRangeStart>0x0</addrRangeStart>"
  echo "      <addrRangeEnd>0xe8000000</addrRangeEnd>"
  echo "      <backingStoreSize>0x8000000</backingStoreSize>"
  echo "    </params>"
  echo "    <link name=dir_mem_link_0 port=memory latency=$BUSLAT />"
  echo "    <link name=dir_net_0 port=network latency=$NETLAT />"
  echo "  </component>"
  echo
  echo "  <component name=\"memory0\" rank=\"0\" type=\"memHierarchy.MemController\">"
  echo "    <params>"
  echo "      <access_time>$DRAM_ATIME</access_time>"
  echo "      <mem_size>4096</mem_size>"
  echo "      <clock>$DRAM_CLOCK</clock>"
  echo "    </params>"
  echo "    <link name=dir_mem_link_0 port=direct_link latency=$BUSLAT />"
  echo "  </component>"
}

print_sep() {
  echo "  <!--<><><><><><><><><><><><><><>-<><><><><><><><><><><><><><>-->"
  echo
}


echo "<?xml version=\"1.0\"?>"
echo "<sdl version=\"2.0\"/>"
echo
echo "<sst>"

M=`expr $N - 1`

# Generate the CPUs, either trivial_cpu or qsim_cpu
for i in `seq 0 $M`; do print_qsim_cpu; done
#for i in `seq 0 $M`; do print_trivial_cpu; done
print_sep

# Generate the L1 cache(s)
for i in `seq 0 $M`; do print_l1_cache; done
print_sep

# Generate the L2 cache(s)
for i in `seq 0 $M`; do print_l2_cache; done
print_sep

# Generate the network, directory, and RAM
print_network
print_dirmem

echo "</sst>"
