#!/usr/bin/perl -w

sub getLink($$) {
    my $a = shift;
    my $b = shift;

    if ($a < $b) {
	return "$a.$b";
    } else{
	return "$b.$a";
    }
}

sub getCNum($$) {
    my $inX = shift;
    my $inY = shift;
    
    return ($x * $inY) + $inX;
}

$x = $ARGV[0];  #num of nodes
$y = $ARGV[1];  # nothing right now
$set = $ARGV[2]; #set
$ranks = $ARGV[3]; #ranks

if ($set == 1) { #
    $interconnectName = "fullnetwork";
    $network_latency = "380ns";
    $network_bandwidth = "524Mbps";
    $commLat = "100ns";
} else {
    printf("Bad Set\n");
    exit(-1);
}



printf("making ${x}x${y}, setup $set, $ranks ranks\n", $x, $y);

$fileN = "${x}x${y}-$set-$ranks";
open(OUT, ">$fileN.xml");

printf(OUT "<?xml version=\"1.0\"?>\n".
	"<sdl version=2.0/>\n".
       "<config>\n".
       "run-mode=both\n".
       "</config>\n");


printf(OUT "<param_include>\n".
    "\t<net_params>\n".
    "\t\t<network_name>$interconnectName</network_name>\n".
    "\t\t<network_latency>$network_latency </network_latency>\n".
    "\t\t<network_bandwidth>$network_bandwidth </network_bandwidth>\n".
    "\t\t<topology_name>xbar</topology_name>\n".
    "\t\t<topology_geometry>$x</topology_geometry>\n".
    "\t</net_params>\n".
    "\t<node_params>\n".
    "\t\t<manager>simple</manager>\n".
    "\t\t<node_name>simple</node_name>\n".
    "\t\t<node_cores>2</node_cores>\n".
    "\t\t<node_mem_latency>200ns</node_mem_latency>\n".
    "\t\t<node_mem_bandwidth>500Mbps</node_mem_bandwidth>\n".
    "\t\t<node_frequency>1e9</node_frequency>\n".
    "\t\t<node_num>$x</node_num>\n".
    "\t\t<nic_name>simple</nic_name>\n".
    "\t\t<logging_string>app,launch,mpicheck</logging_string>\n".
    "\t</node_params>\n".
    "\t<app_params>\n".
    "\t\t<launch_name>instant</launch_name>\n".
    "\t\t<launch_indexing>block</launch_indexing>\n".
    "\t\t<launch_allocation>firstavailable</launch_allocation>\n".
    "\t\t<launch_app1>MPI_testall</launch_app1>\n".
    "\t\t<launch_app1_size>$ranks</launch_app1_size>\n".
    "\t\t<launch_app1_start>1200ms</launch_app1_start>\n".
    "\t</app_params>\n".
    "</param_include>\n");


printf(OUT "<sst>\n");


for ($xi = 0; $xi < $x; ++$xi) {
	

	print OUT<<EOT
  <component name=c$xi type=macro_component.macro_processor>
      <params include=node_params,app_params>
      </params>
	<link name=port$xi port=nic latency=100ns/>
  </component>

EOT
    }



printf(OUT "  <component name=\"network\" type=macro_component.macro_network>\n");

printf(OUT "      <params include=net_params> \n");
printf(OUT "      </params> \n");


for ($xi = 0; $xi < $x; ++$xi) {

printf(OUT "	<link name=port$xi port=port$xi latency=$commLat /> \n");

}


printf(OUT "  </component> \n");



printf(OUT "</sst>");

close(OUT);
