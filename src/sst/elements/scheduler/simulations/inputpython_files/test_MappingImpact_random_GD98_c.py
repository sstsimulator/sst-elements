# scheduler simulation input file
import sst

# Define SST core options
sst.setProgramOption("run-mode", "both")

# Define the simulation components
scheduler = sst.Component("myScheduler",             "scheduler.schedComponent")
scheduler.addParams({
      "traceName" : "test_MappingImpact_GD98_c.sim",
      "machine" : "dragonfly[4,9,4,2,all_to_all,absolute]",
      "coresPerNode" : "2",
      "scheduler" : "easy",
      "allocator" : "random",
      "taskMapper" : "random",
      "timeperdistance" : ".001865[.1569,0.0129]",
      "dMatrixFile" : "none",
      "detailedNetworkSim" : "ON",
      "completedJobsTrace" : "emberCompleted.txt",
      "runningJobsTrace" : "emberRunning.txt"
})

# nodes
n0 = sst.Component("n0", "scheduler.nodeComponent")
n0.addParams({
      "nodeNum" : "0",
})
n1 = sst.Component("n1", "scheduler.nodeComponent")
n1.addParams({
      "nodeNum" : "1",
})
n2 = sst.Component("n2", "scheduler.nodeComponent")
n2.addParams({
      "nodeNum" : "2",
})
n3 = sst.Component("n3", "scheduler.nodeComponent")
n3.addParams({
      "nodeNum" : "3",
})
n4 = sst.Component("n4", "scheduler.nodeComponent")
n4.addParams({
      "nodeNum" : "4",
})
n5 = sst.Component("n5", "scheduler.nodeComponent")
n5.addParams({
      "nodeNum" : "5",
})
n6 = sst.Component("n6", "scheduler.nodeComponent")
n6.addParams({
      "nodeNum" : "6",
})
n7 = sst.Component("n7", "scheduler.nodeComponent")
n7.addParams({
      "nodeNum" : "7",
})
n8 = sst.Component("n8", "scheduler.nodeComponent")
n8.addParams({
      "nodeNum" : "8",
})
n9 = sst.Component("n9", "scheduler.nodeComponent")
n9.addParams({
      "nodeNum" : "9",
})
n10 = sst.Component("n10", "scheduler.nodeComponent")
n10.addParams({
      "nodeNum" : "10",
})
n11 = sst.Component("n11", "scheduler.nodeComponent")
n11.addParams({
      "nodeNum" : "11",
})
n12 = sst.Component("n12", "scheduler.nodeComponent")
n12.addParams({
      "nodeNum" : "12",
})
n13 = sst.Component("n13", "scheduler.nodeComponent")
n13.addParams({
      "nodeNum" : "13",
})
n14 = sst.Component("n14", "scheduler.nodeComponent")
n14.addParams({
      "nodeNum" : "14",
})
n15 = sst.Component("n15", "scheduler.nodeComponent")
n15.addParams({
      "nodeNum" : "15",
})
n16 = sst.Component("n16", "scheduler.nodeComponent")
n16.addParams({
      "nodeNum" : "16",
})
n17 = sst.Component("n17", "scheduler.nodeComponent")
n17.addParams({
      "nodeNum" : "17",
})
n18 = sst.Component("n18", "scheduler.nodeComponent")
n18.addParams({
      "nodeNum" : "18",
})
n19 = sst.Component("n19", "scheduler.nodeComponent")
n19.addParams({
      "nodeNum" : "19",
})
n20 = sst.Component("n20", "scheduler.nodeComponent")
n20.addParams({
      "nodeNum" : "20",
})
n21 = sst.Component("n21", "scheduler.nodeComponent")
n21.addParams({
      "nodeNum" : "21",
})
n22 = sst.Component("n22", "scheduler.nodeComponent")
n22.addParams({
      "nodeNum" : "22",
})
n23 = sst.Component("n23", "scheduler.nodeComponent")
n23.addParams({
      "nodeNum" : "23",
})
n24 = sst.Component("n24", "scheduler.nodeComponent")
n24.addParams({
      "nodeNum" : "24",
})
n25 = sst.Component("n25", "scheduler.nodeComponent")
n25.addParams({
      "nodeNum" : "25",
})
n26 = sst.Component("n26", "scheduler.nodeComponent")
n26.addParams({
      "nodeNum" : "26",
})
n27 = sst.Component("n27", "scheduler.nodeComponent")
n27.addParams({
      "nodeNum" : "27",
})
n28 = sst.Component("n28", "scheduler.nodeComponent")
n28.addParams({
      "nodeNum" : "28",
})
n29 = sst.Component("n29", "scheduler.nodeComponent")
n29.addParams({
      "nodeNum" : "29",
})
n30 = sst.Component("n30", "scheduler.nodeComponent")
n30.addParams({
      "nodeNum" : "30",
})
n31 = sst.Component("n31", "scheduler.nodeComponent")
n31.addParams({
      "nodeNum" : "31",
})
n32 = sst.Component("n32", "scheduler.nodeComponent")
n32.addParams({
      "nodeNum" : "32",
})
n33 = sst.Component("n33", "scheduler.nodeComponent")
n33.addParams({
      "nodeNum" : "33",
})
n34 = sst.Component("n34", "scheduler.nodeComponent")
n34.addParams({
      "nodeNum" : "34",
})
n35 = sst.Component("n35", "scheduler.nodeComponent")
n35.addParams({
      "nodeNum" : "35",
})
n36 = sst.Component("n36", "scheduler.nodeComponent")
n36.addParams({
      "nodeNum" : "36",
})
n37 = sst.Component("n37", "scheduler.nodeComponent")
n37.addParams({
      "nodeNum" : "37",
})
n38 = sst.Component("n38", "scheduler.nodeComponent")
n38.addParams({
      "nodeNum" : "38",
})
n39 = sst.Component("n39", "scheduler.nodeComponent")
n39.addParams({
      "nodeNum" : "39",
})
n40 = sst.Component("n40", "scheduler.nodeComponent")
n40.addParams({
      "nodeNum" : "40",
})
n41 = sst.Component("n41", "scheduler.nodeComponent")
n41.addParams({
      "nodeNum" : "41",
})
n42 = sst.Component("n42", "scheduler.nodeComponent")
n42.addParams({
      "nodeNum" : "42",
})
n43 = sst.Component("n43", "scheduler.nodeComponent")
n43.addParams({
      "nodeNum" : "43",
})
n44 = sst.Component("n44", "scheduler.nodeComponent")
n44.addParams({
      "nodeNum" : "44",
})
n45 = sst.Component("n45", "scheduler.nodeComponent")
n45.addParams({
      "nodeNum" : "45",
})
n46 = sst.Component("n46", "scheduler.nodeComponent")
n46.addParams({
      "nodeNum" : "46",
})
n47 = sst.Component("n47", "scheduler.nodeComponent")
n47.addParams({
      "nodeNum" : "47",
})
n48 = sst.Component("n48", "scheduler.nodeComponent")
n48.addParams({
      "nodeNum" : "48",
})
n49 = sst.Component("n49", "scheduler.nodeComponent")
n49.addParams({
      "nodeNum" : "49",
})
n50 = sst.Component("n50", "scheduler.nodeComponent")
n50.addParams({
      "nodeNum" : "50",
})
n51 = sst.Component("n51", "scheduler.nodeComponent")
n51.addParams({
      "nodeNum" : "51",
})
n52 = sst.Component("n52", "scheduler.nodeComponent")
n52.addParams({
      "nodeNum" : "52",
})
n53 = sst.Component("n53", "scheduler.nodeComponent")
n53.addParams({
      "nodeNum" : "53",
})
n54 = sst.Component("n54", "scheduler.nodeComponent")
n54.addParams({
      "nodeNum" : "54",
})
n55 = sst.Component("n55", "scheduler.nodeComponent")
n55.addParams({
      "nodeNum" : "55",
})
n56 = sst.Component("n56", "scheduler.nodeComponent")
n56.addParams({
      "nodeNum" : "56",
})
n57 = sst.Component("n57", "scheduler.nodeComponent")
n57.addParams({
      "nodeNum" : "57",
})
n58 = sst.Component("n58", "scheduler.nodeComponent")
n58.addParams({
      "nodeNum" : "58",
})
n59 = sst.Component("n59", "scheduler.nodeComponent")
n59.addParams({
      "nodeNum" : "59",
})
n60 = sst.Component("n60", "scheduler.nodeComponent")
n60.addParams({
      "nodeNum" : "60",
})
n61 = sst.Component("n61", "scheduler.nodeComponent")
n61.addParams({
      "nodeNum" : "61",
})
n62 = sst.Component("n62", "scheduler.nodeComponent")
n62.addParams({
      "nodeNum" : "62",
})
n63 = sst.Component("n63", "scheduler.nodeComponent")
n63.addParams({
      "nodeNum" : "63",
})
n64 = sst.Component("n64", "scheduler.nodeComponent")
n64.addParams({
      "nodeNum" : "64",
})
n65 = sst.Component("n65", "scheduler.nodeComponent")
n65.addParams({
      "nodeNum" : "65",
})
n66 = sst.Component("n66", "scheduler.nodeComponent")
n66.addParams({
      "nodeNum" : "66",
})
n67 = sst.Component("n67", "scheduler.nodeComponent")
n67.addParams({
      "nodeNum" : "67",
})
n68 = sst.Component("n68", "scheduler.nodeComponent")
n68.addParams({
      "nodeNum" : "68",
})
n69 = sst.Component("n69", "scheduler.nodeComponent")
n69.addParams({
      "nodeNum" : "69",
})
n70 = sst.Component("n70", "scheduler.nodeComponent")
n70.addParams({
      "nodeNum" : "70",
})
n71 = sst.Component("n71", "scheduler.nodeComponent")
n71.addParams({
      "nodeNum" : "71",
})
n72 = sst.Component("n72", "scheduler.nodeComponent")
n72.addParams({
      "nodeNum" : "72",
})
n73 = sst.Component("n73", "scheduler.nodeComponent")
n73.addParams({
      "nodeNum" : "73",
})
n74 = sst.Component("n74", "scheduler.nodeComponent")
n74.addParams({
      "nodeNum" : "74",
})
n75 = sst.Component("n75", "scheduler.nodeComponent")
n75.addParams({
      "nodeNum" : "75",
})
n76 = sst.Component("n76", "scheduler.nodeComponent")
n76.addParams({
      "nodeNum" : "76",
})
n77 = sst.Component("n77", "scheduler.nodeComponent")
n77.addParams({
      "nodeNum" : "77",
})
n78 = sst.Component("n78", "scheduler.nodeComponent")
n78.addParams({
      "nodeNum" : "78",
})
n79 = sst.Component("n79", "scheduler.nodeComponent")
n79.addParams({
      "nodeNum" : "79",
})
n80 = sst.Component("n80", "scheduler.nodeComponent")
n80.addParams({
      "nodeNum" : "80",
})
n81 = sst.Component("n81", "scheduler.nodeComponent")
n81.addParams({
      "nodeNum" : "81",
})
n82 = sst.Component("n82", "scheduler.nodeComponent")
n82.addParams({
      "nodeNum" : "82",
})
n83 = sst.Component("n83", "scheduler.nodeComponent")
n83.addParams({
      "nodeNum" : "83",
})
n84 = sst.Component("n84", "scheduler.nodeComponent")
n84.addParams({
      "nodeNum" : "84",
})
n85 = sst.Component("n85", "scheduler.nodeComponent")
n85.addParams({
      "nodeNum" : "85",
})
n86 = sst.Component("n86", "scheduler.nodeComponent")
n86.addParams({
      "nodeNum" : "86",
})
n87 = sst.Component("n87", "scheduler.nodeComponent")
n87.addParams({
      "nodeNum" : "87",
})
n88 = sst.Component("n88", "scheduler.nodeComponent")
n88.addParams({
      "nodeNum" : "88",
})
n89 = sst.Component("n89", "scheduler.nodeComponent")
n89.addParams({
      "nodeNum" : "89",
})
n90 = sst.Component("n90", "scheduler.nodeComponent")
n90.addParams({
      "nodeNum" : "90",
})
n91 = sst.Component("n91", "scheduler.nodeComponent")
n91.addParams({
      "nodeNum" : "91",
})
n92 = sst.Component("n92", "scheduler.nodeComponent")
n92.addParams({
      "nodeNum" : "92",
})
n93 = sst.Component("n93", "scheduler.nodeComponent")
n93.addParams({
      "nodeNum" : "93",
})
n94 = sst.Component("n94", "scheduler.nodeComponent")
n94.addParams({
      "nodeNum" : "94",
})
n95 = sst.Component("n95", "scheduler.nodeComponent")
n95.addParams({
      "nodeNum" : "95",
})
n96 = sst.Component("n96", "scheduler.nodeComponent")
n96.addParams({
      "nodeNum" : "96",
})
n97 = sst.Component("n97", "scheduler.nodeComponent")
n97.addParams({
      "nodeNum" : "97",
})
n98 = sst.Component("n98", "scheduler.nodeComponent")
n98.addParams({
      "nodeNum" : "98",
})
n99 = sst.Component("n99", "scheduler.nodeComponent")
n99.addParams({
      "nodeNum" : "99",
})
n100 = sst.Component("n100", "scheduler.nodeComponent")
n100.addParams({
      "nodeNum" : "100",
})
n101 = sst.Component("n101", "scheduler.nodeComponent")
n101.addParams({
      "nodeNum" : "101",
})
n102 = sst.Component("n102", "scheduler.nodeComponent")
n102.addParams({
      "nodeNum" : "102",
})
n103 = sst.Component("n103", "scheduler.nodeComponent")
n103.addParams({
      "nodeNum" : "103",
})
n104 = sst.Component("n104", "scheduler.nodeComponent")
n104.addParams({
      "nodeNum" : "104",
})
n105 = sst.Component("n105", "scheduler.nodeComponent")
n105.addParams({
      "nodeNum" : "105",
})
n106 = sst.Component("n106", "scheduler.nodeComponent")
n106.addParams({
      "nodeNum" : "106",
})
n107 = sst.Component("n107", "scheduler.nodeComponent")
n107.addParams({
      "nodeNum" : "107",
})
n108 = sst.Component("n108", "scheduler.nodeComponent")
n108.addParams({
      "nodeNum" : "108",
})
n109 = sst.Component("n109", "scheduler.nodeComponent")
n109.addParams({
      "nodeNum" : "109",
})
n110 = sst.Component("n110", "scheduler.nodeComponent")
n110.addParams({
      "nodeNum" : "110",
})
n111 = sst.Component("n111", "scheduler.nodeComponent")
n111.addParams({
      "nodeNum" : "111",
})
n112 = sst.Component("n112", "scheduler.nodeComponent")
n112.addParams({
      "nodeNum" : "112",
})
n113 = sst.Component("n113", "scheduler.nodeComponent")
n113.addParams({
      "nodeNum" : "113",
})
n114 = sst.Component("n114", "scheduler.nodeComponent")
n114.addParams({
      "nodeNum" : "114",
})
n115 = sst.Component("n115", "scheduler.nodeComponent")
n115.addParams({
      "nodeNum" : "115",
})
n116 = sst.Component("n116", "scheduler.nodeComponent")
n116.addParams({
      "nodeNum" : "116",
})
n117 = sst.Component("n117", "scheduler.nodeComponent")
n117.addParams({
      "nodeNum" : "117",
})
n118 = sst.Component("n118", "scheduler.nodeComponent")
n118.addParams({
      "nodeNum" : "118",
})
n119 = sst.Component("n119", "scheduler.nodeComponent")
n119.addParams({
      "nodeNum" : "119",
})
n120 = sst.Component("n120", "scheduler.nodeComponent")
n120.addParams({
      "nodeNum" : "120",
})
n121 = sst.Component("n121", "scheduler.nodeComponent")
n121.addParams({
      "nodeNum" : "121",
})
n122 = sst.Component("n122", "scheduler.nodeComponent")
n122.addParams({
      "nodeNum" : "122",
})
n123 = sst.Component("n123", "scheduler.nodeComponent")
n123.addParams({
      "nodeNum" : "123",
})
n124 = sst.Component("n124", "scheduler.nodeComponent")
n124.addParams({
      "nodeNum" : "124",
})
n125 = sst.Component("n125", "scheduler.nodeComponent")
n125.addParams({
      "nodeNum" : "125",
})
n126 = sst.Component("n126", "scheduler.nodeComponent")
n126.addParams({
      "nodeNum" : "126",
})
n127 = sst.Component("n127", "scheduler.nodeComponent")
n127.addParams({
      "nodeNum" : "127",
})
n128 = sst.Component("n128", "scheduler.nodeComponent")
n128.addParams({
      "nodeNum" : "128",
})
n129 = sst.Component("n129", "scheduler.nodeComponent")
n129.addParams({
      "nodeNum" : "129",
})
n130 = sst.Component("n130", "scheduler.nodeComponent")
n130.addParams({
      "nodeNum" : "130",
})
n131 = sst.Component("n131", "scheduler.nodeComponent")
n131.addParams({
      "nodeNum" : "131",
})
n132 = sst.Component("n132", "scheduler.nodeComponent")
n132.addParams({
      "nodeNum" : "132",
})
n133 = sst.Component("n133", "scheduler.nodeComponent")
n133.addParams({
      "nodeNum" : "133",
})
n134 = sst.Component("n134", "scheduler.nodeComponent")
n134.addParams({
      "nodeNum" : "134",
})
n135 = sst.Component("n135", "scheduler.nodeComponent")
n135.addParams({
      "nodeNum" : "135",
})

# define links
l0 = sst.Link("l0")
l0.connect( (scheduler, "nodeLink0", "0 ns"), (n0, "Scheduler", "0 ns") )
l1 = sst.Link("l1")
l1.connect( (scheduler, "nodeLink1", "0 ns"), (n1, "Scheduler", "0 ns") )
l2 = sst.Link("l2")
l2.connect( (scheduler, "nodeLink2", "0 ns"), (n2, "Scheduler", "0 ns") )
l3 = sst.Link("l3")
l3.connect( (scheduler, "nodeLink3", "0 ns"), (n3, "Scheduler", "0 ns") )
l4 = sst.Link("l4")
l4.connect( (scheduler, "nodeLink4", "0 ns"), (n4, "Scheduler", "0 ns") )
l5 = sst.Link("l5")
l5.connect( (scheduler, "nodeLink5", "0 ns"), (n5, "Scheduler", "0 ns") )
l6 = sst.Link("l6")
l6.connect( (scheduler, "nodeLink6", "0 ns"), (n6, "Scheduler", "0 ns") )
l7 = sst.Link("l7")
l7.connect( (scheduler, "nodeLink7", "0 ns"), (n7, "Scheduler", "0 ns") )
l8 = sst.Link("l8")
l8.connect( (scheduler, "nodeLink8", "0 ns"), (n8, "Scheduler", "0 ns") )
l9 = sst.Link("l9")
l9.connect( (scheduler, "nodeLink9", "0 ns"), (n9, "Scheduler", "0 ns") )
l10 = sst.Link("l10")
l10.connect( (scheduler, "nodeLink10", "0 ns"), (n10, "Scheduler", "0 ns") )
l11 = sst.Link("l11")
l11.connect( (scheduler, "nodeLink11", "0 ns"), (n11, "Scheduler", "0 ns") )
l12 = sst.Link("l12")
l12.connect( (scheduler, "nodeLink12", "0 ns"), (n12, "Scheduler", "0 ns") )
l13 = sst.Link("l13")
l13.connect( (scheduler, "nodeLink13", "0 ns"), (n13, "Scheduler", "0 ns") )
l14 = sst.Link("l14")
l14.connect( (scheduler, "nodeLink14", "0 ns"), (n14, "Scheduler", "0 ns") )
l15 = sst.Link("l15")
l15.connect( (scheduler, "nodeLink15", "0 ns"), (n15, "Scheduler", "0 ns") )
l16 = sst.Link("l16")
l16.connect( (scheduler, "nodeLink16", "0 ns"), (n16, "Scheduler", "0 ns") )
l17 = sst.Link("l17")
l17.connect( (scheduler, "nodeLink17", "0 ns"), (n17, "Scheduler", "0 ns") )
l18 = sst.Link("l18")
l18.connect( (scheduler, "nodeLink18", "0 ns"), (n18, "Scheduler", "0 ns") )
l19 = sst.Link("l19")
l19.connect( (scheduler, "nodeLink19", "0 ns"), (n19, "Scheduler", "0 ns") )
l20 = sst.Link("l20")
l20.connect( (scheduler, "nodeLink20", "0 ns"), (n20, "Scheduler", "0 ns") )
l21 = sst.Link("l21")
l21.connect( (scheduler, "nodeLink21", "0 ns"), (n21, "Scheduler", "0 ns") )
l22 = sst.Link("l22")
l22.connect( (scheduler, "nodeLink22", "0 ns"), (n22, "Scheduler", "0 ns") )
l23 = sst.Link("l23")
l23.connect( (scheduler, "nodeLink23", "0 ns"), (n23, "Scheduler", "0 ns") )
l24 = sst.Link("l24")
l24.connect( (scheduler, "nodeLink24", "0 ns"), (n24, "Scheduler", "0 ns") )
l25 = sst.Link("l25")
l25.connect( (scheduler, "nodeLink25", "0 ns"), (n25, "Scheduler", "0 ns") )
l26 = sst.Link("l26")
l26.connect( (scheduler, "nodeLink26", "0 ns"), (n26, "Scheduler", "0 ns") )
l27 = sst.Link("l27")
l27.connect( (scheduler, "nodeLink27", "0 ns"), (n27, "Scheduler", "0 ns") )
l28 = sst.Link("l28")
l28.connect( (scheduler, "nodeLink28", "0 ns"), (n28, "Scheduler", "0 ns") )
l29 = sst.Link("l29")
l29.connect( (scheduler, "nodeLink29", "0 ns"), (n29, "Scheduler", "0 ns") )
l30 = sst.Link("l30")
l30.connect( (scheduler, "nodeLink30", "0 ns"), (n30, "Scheduler", "0 ns") )
l31 = sst.Link("l31")
l31.connect( (scheduler, "nodeLink31", "0 ns"), (n31, "Scheduler", "0 ns") )
l32 = sst.Link("l32")
l32.connect( (scheduler, "nodeLink32", "0 ns"), (n32, "Scheduler", "0 ns") )
l33 = sst.Link("l33")
l33.connect( (scheduler, "nodeLink33", "0 ns"), (n33, "Scheduler", "0 ns") )
l34 = sst.Link("l34")
l34.connect( (scheduler, "nodeLink34", "0 ns"), (n34, "Scheduler", "0 ns") )
l35 = sst.Link("l35")
l35.connect( (scheduler, "nodeLink35", "0 ns"), (n35, "Scheduler", "0 ns") )
l36 = sst.Link("l36")
l36.connect( (scheduler, "nodeLink36", "0 ns"), (n36, "Scheduler", "0 ns") )
l37 = sst.Link("l37")
l37.connect( (scheduler, "nodeLink37", "0 ns"), (n37, "Scheduler", "0 ns") )
l38 = sst.Link("l38")
l38.connect( (scheduler, "nodeLink38", "0 ns"), (n38, "Scheduler", "0 ns") )
l39 = sst.Link("l39")
l39.connect( (scheduler, "nodeLink39", "0 ns"), (n39, "Scheduler", "0 ns") )
l40 = sst.Link("l40")
l40.connect( (scheduler, "nodeLink40", "0 ns"), (n40, "Scheduler", "0 ns") )
l41 = sst.Link("l41")
l41.connect( (scheduler, "nodeLink41", "0 ns"), (n41, "Scheduler", "0 ns") )
l42 = sst.Link("l42")
l42.connect( (scheduler, "nodeLink42", "0 ns"), (n42, "Scheduler", "0 ns") )
l43 = sst.Link("l43")
l43.connect( (scheduler, "nodeLink43", "0 ns"), (n43, "Scheduler", "0 ns") )
l44 = sst.Link("l44")
l44.connect( (scheduler, "nodeLink44", "0 ns"), (n44, "Scheduler", "0 ns") )
l45 = sst.Link("l45")
l45.connect( (scheduler, "nodeLink45", "0 ns"), (n45, "Scheduler", "0 ns") )
l46 = sst.Link("l46")
l46.connect( (scheduler, "nodeLink46", "0 ns"), (n46, "Scheduler", "0 ns") )
l47 = sst.Link("l47")
l47.connect( (scheduler, "nodeLink47", "0 ns"), (n47, "Scheduler", "0 ns") )
l48 = sst.Link("l48")
l48.connect( (scheduler, "nodeLink48", "0 ns"), (n48, "Scheduler", "0 ns") )
l49 = sst.Link("l49")
l49.connect( (scheduler, "nodeLink49", "0 ns"), (n49, "Scheduler", "0 ns") )
l50 = sst.Link("l50")
l50.connect( (scheduler, "nodeLink50", "0 ns"), (n50, "Scheduler", "0 ns") )
l51 = sst.Link("l51")
l51.connect( (scheduler, "nodeLink51", "0 ns"), (n51, "Scheduler", "0 ns") )
l52 = sst.Link("l52")
l52.connect( (scheduler, "nodeLink52", "0 ns"), (n52, "Scheduler", "0 ns") )
l53 = sst.Link("l53")
l53.connect( (scheduler, "nodeLink53", "0 ns"), (n53, "Scheduler", "0 ns") )
l54 = sst.Link("l54")
l54.connect( (scheduler, "nodeLink54", "0 ns"), (n54, "Scheduler", "0 ns") )
l55 = sst.Link("l55")
l55.connect( (scheduler, "nodeLink55", "0 ns"), (n55, "Scheduler", "0 ns") )
l56 = sst.Link("l56")
l56.connect( (scheduler, "nodeLink56", "0 ns"), (n56, "Scheduler", "0 ns") )
l57 = sst.Link("l57")
l57.connect( (scheduler, "nodeLink57", "0 ns"), (n57, "Scheduler", "0 ns") )
l58 = sst.Link("l58")
l58.connect( (scheduler, "nodeLink58", "0 ns"), (n58, "Scheduler", "0 ns") )
l59 = sst.Link("l59")
l59.connect( (scheduler, "nodeLink59", "0 ns"), (n59, "Scheduler", "0 ns") )
l60 = sst.Link("l60")
l60.connect( (scheduler, "nodeLink60", "0 ns"), (n60, "Scheduler", "0 ns") )
l61 = sst.Link("l61")
l61.connect( (scheduler, "nodeLink61", "0 ns"), (n61, "Scheduler", "0 ns") )
l62 = sst.Link("l62")
l62.connect( (scheduler, "nodeLink62", "0 ns"), (n62, "Scheduler", "0 ns") )
l63 = sst.Link("l63")
l63.connect( (scheduler, "nodeLink63", "0 ns"), (n63, "Scheduler", "0 ns") )
l64 = sst.Link("l64")
l64.connect( (scheduler, "nodeLink64", "0 ns"), (n64, "Scheduler", "0 ns") )
l65 = sst.Link("l65")
l65.connect( (scheduler, "nodeLink65", "0 ns"), (n65, "Scheduler", "0 ns") )
l66 = sst.Link("l66")
l66.connect( (scheduler, "nodeLink66", "0 ns"), (n66, "Scheduler", "0 ns") )
l67 = sst.Link("l67")
l67.connect( (scheduler, "nodeLink67", "0 ns"), (n67, "Scheduler", "0 ns") )
l68 = sst.Link("l68")
l68.connect( (scheduler, "nodeLink68", "0 ns"), (n68, "Scheduler", "0 ns") )
l69 = sst.Link("l69")
l69.connect( (scheduler, "nodeLink69", "0 ns"), (n69, "Scheduler", "0 ns") )
l70 = sst.Link("l70")
l70.connect( (scheduler, "nodeLink70", "0 ns"), (n70, "Scheduler", "0 ns") )
l71 = sst.Link("l71")
l71.connect( (scheduler, "nodeLink71", "0 ns"), (n71, "Scheduler", "0 ns") )
l72 = sst.Link("l72")
l72.connect( (scheduler, "nodeLink72", "0 ns"), (n72, "Scheduler", "0 ns") )
l73 = sst.Link("l73")
l73.connect( (scheduler, "nodeLink73", "0 ns"), (n73, "Scheduler", "0 ns") )
l74 = sst.Link("l74")
l74.connect( (scheduler, "nodeLink74", "0 ns"), (n74, "Scheduler", "0 ns") )
l75 = sst.Link("l75")
l75.connect( (scheduler, "nodeLink75", "0 ns"), (n75, "Scheduler", "0 ns") )
l76 = sst.Link("l76")
l76.connect( (scheduler, "nodeLink76", "0 ns"), (n76, "Scheduler", "0 ns") )
l77 = sst.Link("l77")
l77.connect( (scheduler, "nodeLink77", "0 ns"), (n77, "Scheduler", "0 ns") )
l78 = sst.Link("l78")
l78.connect( (scheduler, "nodeLink78", "0 ns"), (n78, "Scheduler", "0 ns") )
l79 = sst.Link("l79")
l79.connect( (scheduler, "nodeLink79", "0 ns"), (n79, "Scheduler", "0 ns") )
l80 = sst.Link("l80")
l80.connect( (scheduler, "nodeLink80", "0 ns"), (n80, "Scheduler", "0 ns") )
l81 = sst.Link("l81")
l81.connect( (scheduler, "nodeLink81", "0 ns"), (n81, "Scheduler", "0 ns") )
l82 = sst.Link("l82")
l82.connect( (scheduler, "nodeLink82", "0 ns"), (n82, "Scheduler", "0 ns") )
l83 = sst.Link("l83")
l83.connect( (scheduler, "nodeLink83", "0 ns"), (n83, "Scheduler", "0 ns") )
l84 = sst.Link("l84")
l84.connect( (scheduler, "nodeLink84", "0 ns"), (n84, "Scheduler", "0 ns") )
l85 = sst.Link("l85")
l85.connect( (scheduler, "nodeLink85", "0 ns"), (n85, "Scheduler", "0 ns") )
l86 = sst.Link("l86")
l86.connect( (scheduler, "nodeLink86", "0 ns"), (n86, "Scheduler", "0 ns") )
l87 = sst.Link("l87")
l87.connect( (scheduler, "nodeLink87", "0 ns"), (n87, "Scheduler", "0 ns") )
l88 = sst.Link("l88")
l88.connect( (scheduler, "nodeLink88", "0 ns"), (n88, "Scheduler", "0 ns") )
l89 = sst.Link("l89")
l89.connect( (scheduler, "nodeLink89", "0 ns"), (n89, "Scheduler", "0 ns") )
l90 = sst.Link("l90")
l90.connect( (scheduler, "nodeLink90", "0 ns"), (n90, "Scheduler", "0 ns") )
l91 = sst.Link("l91")
l91.connect( (scheduler, "nodeLink91", "0 ns"), (n91, "Scheduler", "0 ns") )
l92 = sst.Link("l92")
l92.connect( (scheduler, "nodeLink92", "0 ns"), (n92, "Scheduler", "0 ns") )
l93 = sst.Link("l93")
l93.connect( (scheduler, "nodeLink93", "0 ns"), (n93, "Scheduler", "0 ns") )
l94 = sst.Link("l94")
l94.connect( (scheduler, "nodeLink94", "0 ns"), (n94, "Scheduler", "0 ns") )
l95 = sst.Link("l95")
l95.connect( (scheduler, "nodeLink95", "0 ns"), (n95, "Scheduler", "0 ns") )
l96 = sst.Link("l96")
l96.connect( (scheduler, "nodeLink96", "0 ns"), (n96, "Scheduler", "0 ns") )
l97 = sst.Link("l97")
l97.connect( (scheduler, "nodeLink97", "0 ns"), (n97, "Scheduler", "0 ns") )
l98 = sst.Link("l98")
l98.connect( (scheduler, "nodeLink98", "0 ns"), (n98, "Scheduler", "0 ns") )
l99 = sst.Link("l99")
l99.connect( (scheduler, "nodeLink99", "0 ns"), (n99, "Scheduler", "0 ns") )
l100 = sst.Link("l100")
l100.connect( (scheduler, "nodeLink100", "0 ns"), (n100, "Scheduler", "0 ns") )
l101 = sst.Link("l101")
l101.connect( (scheduler, "nodeLink101", "0 ns"), (n101, "Scheduler", "0 ns") )
l102 = sst.Link("l102")
l102.connect( (scheduler, "nodeLink102", "0 ns"), (n102, "Scheduler", "0 ns") )
l103 = sst.Link("l103")
l103.connect( (scheduler, "nodeLink103", "0 ns"), (n103, "Scheduler", "0 ns") )
l104 = sst.Link("l104")
l104.connect( (scheduler, "nodeLink104", "0 ns"), (n104, "Scheduler", "0 ns") )
l105 = sst.Link("l105")
l105.connect( (scheduler, "nodeLink105", "0 ns"), (n105, "Scheduler", "0 ns") )
l106 = sst.Link("l106")
l106.connect( (scheduler, "nodeLink106", "0 ns"), (n106, "Scheduler", "0 ns") )
l107 = sst.Link("l107")
l107.connect( (scheduler, "nodeLink107", "0 ns"), (n107, "Scheduler", "0 ns") )
l108 = sst.Link("l108")
l108.connect( (scheduler, "nodeLink108", "0 ns"), (n108, "Scheduler", "0 ns") )
l109 = sst.Link("l109")
l109.connect( (scheduler, "nodeLink109", "0 ns"), (n109, "Scheduler", "0 ns") )
l110 = sst.Link("l110")
l110.connect( (scheduler, "nodeLink110", "0 ns"), (n110, "Scheduler", "0 ns") )
l111 = sst.Link("l111")
l111.connect( (scheduler, "nodeLink111", "0 ns"), (n111, "Scheduler", "0 ns") )
l112 = sst.Link("l112")
l112.connect( (scheduler, "nodeLink112", "0 ns"), (n112, "Scheduler", "0 ns") )
l113 = sst.Link("l113")
l113.connect( (scheduler, "nodeLink113", "0 ns"), (n113, "Scheduler", "0 ns") )
l114 = sst.Link("l114")
l114.connect( (scheduler, "nodeLink114", "0 ns"), (n114, "Scheduler", "0 ns") )
l115 = sst.Link("l115")
l115.connect( (scheduler, "nodeLink115", "0 ns"), (n115, "Scheduler", "0 ns") )
l116 = sst.Link("l116")
l116.connect( (scheduler, "nodeLink116", "0 ns"), (n116, "Scheduler", "0 ns") )
l117 = sst.Link("l117")
l117.connect( (scheduler, "nodeLink117", "0 ns"), (n117, "Scheduler", "0 ns") )
l118 = sst.Link("l118")
l118.connect( (scheduler, "nodeLink118", "0 ns"), (n118, "Scheduler", "0 ns") )
l119 = sst.Link("l119")
l119.connect( (scheduler, "nodeLink119", "0 ns"), (n119, "Scheduler", "0 ns") )
l120 = sst.Link("l120")
l120.connect( (scheduler, "nodeLink120", "0 ns"), (n120, "Scheduler", "0 ns") )
l121 = sst.Link("l121")
l121.connect( (scheduler, "nodeLink121", "0 ns"), (n121, "Scheduler", "0 ns") )
l122 = sst.Link("l122")
l122.connect( (scheduler, "nodeLink122", "0 ns"), (n122, "Scheduler", "0 ns") )
l123 = sst.Link("l123")
l123.connect( (scheduler, "nodeLink123", "0 ns"), (n123, "Scheduler", "0 ns") )
l124 = sst.Link("l124")
l124.connect( (scheduler, "nodeLink124", "0 ns"), (n124, "Scheduler", "0 ns") )
l125 = sst.Link("l125")
l125.connect( (scheduler, "nodeLink125", "0 ns"), (n125, "Scheduler", "0 ns") )
l126 = sst.Link("l126")
l126.connect( (scheduler, "nodeLink126", "0 ns"), (n126, "Scheduler", "0 ns") )
l127 = sst.Link("l127")
l127.connect( (scheduler, "nodeLink127", "0 ns"), (n127, "Scheduler", "0 ns") )
l128 = sst.Link("l128")
l128.connect( (scheduler, "nodeLink128", "0 ns"), (n128, "Scheduler", "0 ns") )
l129 = sst.Link("l129")
l129.connect( (scheduler, "nodeLink129", "0 ns"), (n129, "Scheduler", "0 ns") )
l130 = sst.Link("l130")
l130.connect( (scheduler, "nodeLink130", "0 ns"), (n130, "Scheduler", "0 ns") )
l131 = sst.Link("l131")
l131.connect( (scheduler, "nodeLink131", "0 ns"), (n131, "Scheduler", "0 ns") )
l132 = sst.Link("l132")
l132.connect( (scheduler, "nodeLink132", "0 ns"), (n132, "Scheduler", "0 ns") )
l133 = sst.Link("l133")
l133.connect( (scheduler, "nodeLink133", "0 ns"), (n133, "Scheduler", "0 ns") )
l134 = sst.Link("l134")
l134.connect( (scheduler, "nodeLink134", "0 ns"), (n134, "Scheduler", "0 ns") )
l135 = sst.Link("l135")
l135.connect( (scheduler, "nodeLink135", "0 ns"), (n135, "Scheduler", "0 ns") )

