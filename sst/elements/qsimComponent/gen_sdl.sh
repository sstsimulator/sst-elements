#!/bin/bash

N=${N:=1}
APP=${APP:=$HOME/sst-simulator/qsim/benchmarks/splash2-tar/fft.tar}
STATE=${STATE:=$HOME/qsim/state.$N}
L1LAT=100ps
IPILAT=10ns
BUSLAT=10ns
NETLAT=5ns

CACHECLOCK=${CACHECLOCK:=1GHz}
COHERENCEPROTO=${COHERENCEPROTO:=MSI}

CACHEBLOCK=${CACHEBLOCK:=64}
L1WAYS=${L1WAYS:=4}
L1SIZE=${L1SIZE:=8}
L1ATIME=${L1ATIME:=1}

L2WAYS=${L2WAYS:=16}
L2SIZE=${L2SIZE:=64}
L2ATIME=${L2ATIME:=10}

RTR_LINK_BW=${RTR_LINK_BW:=2GB/s}
RTR_XBAR_BW=${RTR_XBAR_BW:=2GB/s}
NET_BW=${NET_BW:=2GB/s}

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
  echo "      <num_loadstore> 1000 </num_loadstore>"
  echo "    </params>"
  echo "    <link name=c${i}_l1cache_link port=mem_link latency=$L1LAT />"
  echo "  </component>"
  echo
}

print_l1_cache() {
  echo "  <component name=\"c${i}.l1cache\" rank=\"$i\" type=\"memHierarchy.Cache\">"
  echo "    <params>"
  echo "      <coherence_protocol>$COHERENCEPROTO</coherence_protocol>"
  echo "      <cache_frequency>$CACHECLOCK</cache_frequency>"
  echo "      <replacement_policy> LRU </replacement_policy>"
  echo "      <associativity>$L1WAYS</associativity>"
  echo "      <cache_size>$L1SIZE KB</cache_size>"
  echo "      <cache_line_size>$CACHEBLOCK</cache_line_size>"
  echo "      <access_latency_cycles>$L1ATIME</access_latency_cycles>"
  echo "      <low_network_links> 1 </low_network_links>"
  echo "      <L1>1</L1>"
  echo "      <statistics>1</statistics>"
  echo "    </params>"
  echo "    <link name=c${i}_l1cache_link port=high_network_0 latency=$L1LAT />"
  echo "    <link name=c${i}_l1l2cache_link port=low_network_0 latency=$BUSLAT />"
  echo "  </component>"
  echo
}

print_l2_cache() {
  j=`expr $i + 1`

  echo "  <component name=\"c${i}.l2cache\" rank=\"$i\" type=\"memHierarchy.Cache\">"
  echo "    <params>"
  echo "      <coherence_protocol>$COHERENCEPROTO</coherence_protocol>"
  echo "      <cache_frequency>$CACHECLOCK</cache_frequency>"
  echo "      <replacement_policy> LRU </replacement_policy>"
  echo "      <associativity>$L2WAYS</associativity>"
  echo "      <cache_size>$L2SIZE KB</cache_size>"
  echo "      <cache_line_size>$CACHEBLOCK</cache_line_size>"
  echo "      <access_latency_cycles>$L2ATIME</access_latency_cycles>"
  echo "      <high_network_links> 1 </high_network_links>"
  echo "      <L1>0</L1>"
  echo "      <directory_at_next_level> 1 </directory_at_next_level>"
  echo "      <network_address>$j</network_address>"
  echo "      <network_bw>$NET_BW</network_bw>"
  echo "      <mode>INCLUSIVE</mode>"
  echo "      <printStats>1</printStats>"
  echo "    </params>"
  echo "    <link name=c${i}_l1l2cache_link port=high_network_0 latency=$L1LAT />"
  echo "    <link name=cache_net_${i} port=directory latency=$BUSLAT />"
  echo "  </component>"
  echo
}

print_network() {
  O=`expr $N + 1`

  echo "  <component name=\"chiprtr\" rank=\"0\" type=\"merlin.hr_router\">"
  echo "    <params>"
  echo "      <num_ports>$O</num_ports>"
  echo "      <link_bw>$RTR_LINK_BW</link_bw>"
  echo "      <xbar_bw>$RTR_XBAR_BW</xbar_bw>"
  echo "      <flit_size> 72B </flit_size>"
  echo "      <input_buf_size> 1KB </input_buf_size>"
  echo "      <output_buf_size> 1KB </output_buf_size>"
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
  echo "      <coherence_protocol>$COHERENCEPROTO</coherence_protocol>"
  echo "      <network_bw>$NET_BW</network_bw>"
  echo "      <network_address>0</network_address>"
  echo "      <addr_range_start>0x0</addr_range_start>"
  echo "      <addr_range_end>0xe8000000</addr_range_end>"
  echo "      <backingStoreSize>0</backingStoreSize>"
  echo "    </params>"
  echo "    <link name=dir_mem_link_0 port=memory latency=$BUSLAT />"
  echo "    <link name=dir_net_0 port=network latency=$NETLAT />"
  echo "  </component>"
  echo
  echo "  <component name=\"memory0\" rank=\"0\" type=\"memHierarchy.MemController\">"
  echo "    <params>"
  echo "      <coherence_protocol>$COHERENCEPROTO</coherence_protocol>"
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
