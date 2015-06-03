# scheduler simulation input file
import sst
from sst.merlin import *

# Define SST core options
sst.setProgramOption("run-mode", "both")

# Define the simulation components
scheduler = sst.Component("myScheduler",             "scheduler.schedComponent")
scheduler.addParams({
      "traceName" : "test_scheduler_Atlas.sim",
      "machine" : "mesh[5,4,4]",
      "coresPerNode" : "4",
      "scheduler" : "easy",
      "allocator" : "bestfit",
      "timeperdistance" : ".001865[.1569,0.0129]",
      "dMatrixFile" : "none"
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

# dummy nodes
d0 = sst.Component("n0", "scheduler.dummyComponent")
d0.addParams({
      "dummyNodeNum" : "0",
})
d1 = sst.Component("n1", "scheduler.dummyComponent")
d1.addParams({
      "dummyNodeNum" : "1",
})
d2 = sst.Component("n2", "scheduler.dummyComponent")
d2.addParams({
      "dummyNodeNum" : "2",
})
d3 = sst.Component("n3", "scheduler.dummyComponent")
d3.addParams({
      "dummyNodeNum" : "3",
})
d4 = sst.Component("n4", "scheduler.dummyComponent")
d4.addParams({
      "dummyNodeNum" : "4",
})
d5 = sst.Component("n5", "scheduler.dummyComponent")
d5.addParams({
      "dummyNodeNum" : "5",
})
d6 = sst.Component("n6", "scheduler.dummyComponent")
d6.addParams({
      "dummyNodeNum" : "6",
})
d7 = sst.Component("n7", "scheduler.dummyComponent")
d7.addParams({
      "dummyNodeNum" : "7",
})
d8 = sst.Component("n8", "scheduler.dummyComponent")
d8.addParams({
      "dummyNodeNum" : "8",
})
d9 = sst.Component("n9", "scheduler.dummyComponent")
d9.addParams({
      "dummyNodeNum" : "9",
})
d10 = sst.Component("n10", "scheduler.dummyComponent")
d10.addParams({
      "dummyNodeNum" : "10",
})
d11 = sst.Component("n11", "scheduler.dummyComponent")
d11.addParams({
      "dummyNodeNum" : "11",
})
d12 = sst.Component("n12", "scheduler.dummyComponent")
d12.addParams({
      "dummyNodeNum" : "12",
})
d13 = sst.Component("n13", "scheduler.dummyComponent")
d13.addParams({
      "dummyNodeNum" : "13",
})
d14 = sst.Component("n14", "scheduler.dummyComponent")
d14.addParams({
      "dummyNodeNum" : "14",
})
d15 = sst.Component("n15", "scheduler.dummyComponent")
d15.addParams({
      "dummyNodeNum" : "15",
})
d16 = sst.Component("n16", "scheduler.dummyComponent")
d16.addParams({
      "dummyNodeNum" : "16",
})
d17 = sst.Component("n17", "scheduler.dummyComponent")
d17.addParams({
      "dummyNodeNum" : "17",
})
d18 = sst.Component("n18", "scheduler.dummyComponent")
d18.addParams({
      "dummyNodeNum" : "18",
})
d19 = sst.Component("n19", "scheduler.dummyComponent")
d19.addParams({
      "dummyNodeNum" : "19",
})
d20 = sst.Component("n20", "scheduler.dummyComponent")
d20.addParams({
      "dummyNodeNum" : "20",
})
d21 = sst.Component("n21", "scheduler.dummyComponent")
d21.addParams({
      "dummyNodeNum" : "21",
})
d22 = sst.Component("n22", "scheduler.dummyComponent")
d22.addParams({
      "dummyNodeNum" : "22",
})
d23 = sst.Component("n23", "scheduler.dummyComponent")
d23.addParams({
      "dummyNodeNum" : "23",
})
d24 = sst.Component("n24", "scheduler.dummyComponent")
d24.addParams({
      "dummyNodeNum" : "24",
})
d25 = sst.Component("n25", "scheduler.dummyComponent")
d25.addParams({
      "dummyNodeNum" : "25",
})
d26 = sst.Component("n26", "scheduler.dummyComponent")
d26.addParams({
      "dummyNodeNum" : "26",
})
d27 = sst.Component("n27", "scheduler.dummyComponent")
d27.addParams({
      "dummyNodeNum" : "27",
})
d28 = sst.Component("n28", "scheduler.dummyComponent")
d28.addParams({
      "dummyNodeNum" : "28",
})
d29 = sst.Component("n29", "scheduler.dummyComponent")
d29.addParams({
      "dummyNodeNum" : "29",
})
d30 = sst.Component("n30", "scheduler.dummyComponent")
d30.addParams({
      "dummyNodeNum" : "30",
})
d31 = sst.Component("n31", "scheduler.dummyComponent")
d31.addParams({
      "dummyNodeNum" : "31",
})
d32 = sst.Component("n32", "scheduler.dummyComponent")
d32.addParams({
      "dummyNodeNum" : "32",
})
d33 = sst.Component("n33", "scheduler.dummyComponent")
d33.addParams({
      "dummyNodeNum" : "33",
})
d34 = sst.Component("n34", "scheduler.dummyComponent")
d34.addParams({
      "dummyNodeNum" : "34",
})
d35 = sst.Component("n35", "scheduler.dummyComponent")
d35.addParams({
      "dummyNodeNum" : "35",
})
d36 = sst.Component("n36", "scheduler.dummyComponent")
d36.addParams({
      "dummyNodeNum" : "36",
})
d37 = sst.Component("n37", "scheduler.dummyComponent")
d37.addParams({
      "dummyNodeNum" : "37",
})
d38 = sst.Component("n38", "scheduler.dummyComponent")
d38.addParams({
      "dummyNodeNum" : "38",
})
d39 = sst.Component("n39", "scheduler.dummyComponent")
d39.addParams({
      "dummyNodeNum" : "39",
})
d40 = sst.Component("n40", "scheduler.dummyComponent")
d40.addParams({
      "dummyNodeNum" : "40",
})
d41 = sst.Component("n41", "scheduler.dummyComponent")
d41.addParams({
      "dummyNodeNum" : "41",
})
d42 = sst.Component("n42", "scheduler.dummyComponent")
d42.addParams({
      "dummyNodeNum" : "42",
})
d43 = sst.Component("n43", "scheduler.dummyComponent")
d43.addParams({
      "dummyNodeNum" : "43",
})
d44 = sst.Component("n44", "scheduler.dummyComponent")
d44.addParams({
      "dummyNodeNum" : "44",
})
d45 = sst.Component("n45", "scheduler.dummyComponent")
d45.addParams({
      "dummyNodeNum" : "45",
})
d46 = sst.Component("n46", "scheduler.dummyComponent")
d46.addParams({
      "dummyNodeNum" : "46",
})
d47 = sst.Component("n47", "scheduler.dummyComponent")
d47.addParams({
      "dummyNodeNum" : "47",
})
d48 = sst.Component("n48", "scheduler.dummyComponent")
d48.addParams({
      "dummyNodeNum" : "48",
})
d49 = sst.Component("n49", "scheduler.dummyComponent")
d49.addParams({
      "dummyNodeNum" : "49",
})
d50 = sst.Component("n50", "scheduler.dummyComponent")
d50.addParams({
      "dummyNodeNum" : "50",
})
d51 = sst.Component("n51", "scheduler.dummyComponent")
d51.addParams({
      "dummyNodeNum" : "51",
})
d52 = sst.Component("n52", "scheduler.dummyComponent")
d52.addParams({
      "dummyNodeNum" : "52",
})
d53 = sst.Component("n53", "scheduler.dummyComponent")
d53.addParams({
      "dummyNodeNum" : "53",
})
d54 = sst.Component("n54", "scheduler.dummyComponent")
d54.addParams({
      "dummyNodeNum" : "54",
})
d55 = sst.Component("n55", "scheduler.dummyComponent")
d55.addParams({
      "dummyNodeNum" : "55",
})
d56 = sst.Component("n56", "scheduler.dummyComponent")
d56.addParams({
      "dummyNodeNum" : "56",
})
d57 = sst.Component("n57", "scheduler.dummyComponent")
d57.addParams({
      "dummyNodeNum" : "57",
})
d58 = sst.Component("n58", "scheduler.dummyComponent")
d58.addParams({
      "dummyNodeNum" : "58",
})
d59 = sst.Component("n59", "scheduler.dummyComponent")
d59.addParams({
      "dummyNodeNum" : "59",
})
d60 = sst.Component("n60", "scheduler.dummyComponent")
d60.addParams({
      "dummyNodeNum" : "60",
})
d61 = sst.Component("n61", "scheduler.dummyComponent")
d61.addParams({
      "dummyNodeNum" : "61",
})
d62 = sst.Component("n62", "scheduler.dummyComponent")
d62.addParams({
      "dummyNodeNum" : "62",
})
d63 = sst.Component("n63", "scheduler.dummyComponent")
d63.addParams({
      "dummyNodeNum" : "63",
})
d64 = sst.Component("n64", "scheduler.dummyComponent")
d64.addParams({
      "dummyNodeNum" : "64",
})
d65 = sst.Component("n65", "scheduler.dummyComponent")
d65.addParams({
      "dummyNodeNum" : "65",
})
d66 = sst.Component("n66", "scheduler.dummyComponent")
d66.addParams({
      "dummyNodeNum" : "66",
})
d67 = sst.Component("n67", "scheduler.dummyComponent")
d67.addParams({
      "dummyNodeNum" : "67",
})
d68 = sst.Component("n68", "scheduler.dummyComponent")
d68.addParams({
      "dummyNodeNum" : "68",
})
d69 = sst.Component("n69", "scheduler.dummyComponent")
d69.addParams({
      "dummyNodeNum" : "69",
})
d70 = sst.Component("n70", "scheduler.dummyComponent")
d70.addParams({
      "dummyNodeNum" : "70",
})
d71 = sst.Component("n71", "scheduler.dummyComponent")
d71.addParams({
      "dummyNodeNum" : "71",
})
d72 = sst.Component("n72", "scheduler.dummyComponent")
d72.addParams({
      "dummyNodeNum" : "72",
})
d73 = sst.Component("n73", "scheduler.dummyComponent")
d73.addParams({
      "dummyNodeNum" : "73",
})
d74 = sst.Component("n74", "scheduler.dummyComponent")
d74.addParams({
      "dummyNodeNum" : "74",
})
d75 = sst.Component("n75", "scheduler.dummyComponent")
d75.addParams({
      "dummyNodeNum" : "75",
})
d76 = sst.Component("n76", "scheduler.dummyComponent")
d76.addParams({
      "dummyNodeNum" : "76",
})
d77 = sst.Component("n77", "scheduler.dummyComponent")
d77.addParams({
      "dummyNodeNum" : "77",
})
d78 = sst.Component("n78", "scheduler.dummyComponent")
d78.addParams({
      "dummyNodeNum" : "78",
})
d79 = sst.Component("n79", "scheduler.dummyComponent")
d79.addParams({
      "dummyNodeNum" : "79",
})

# define links between nodes and dummy nodes
dl0 = sst.Link("dl0")
dl0.connect( (n0, "NicLink", "0 ns"), (d0, "nodeLink", "0 ns") )
dl1 = sst.Link("dl1")
dl1.connect( (n1, "NicLink", "0 ns"), (d1, "nodeLink", "0 ns") )
dl2 = sst.Link("dl2")
dl2.connect( (n2, "NicLink", "0 ns"), (d2, "nodeLink", "0 ns") )
dl3 = sst.Link("dl3")
dl3.connect( (n3, "NicLink", "0 ns"), (d3, "nodeLink", "0 ns") )
dl4 = sst.Link("dl4")
dl4.connect( (n4, "NicLink", "0 ns"), (d4, "nodeLink", "0 ns") )
dl5 = sst.Link("dl5")
dl5.connect( (n5, "NicLink", "0 ns"), (d5, "nodeLink", "0 ns") )
dl6 = sst.Link("dl6")
dl6.connect( (n6, "NicLink", "0 ns"), (d6, "nodeLink", "0 ns") )
dl7 = sst.Link("dl7")
dl7.connect( (n7, "NicLink", "0 ns"), (d7, "nodeLink", "0 ns") )
dl8 = sst.Link("dl8")
dl8.connect( (n8, "NicLink", "0 ns"), (d8, "nodeLink", "0 ns") )
dl9 = sst.Link("dl9")
dl9.connect( (n9, "NicLink", "0 ns"), (d9, "nodeLink", "0 ns") )
dl10 = sst.Link("dl10")
dl10.connect( (n10, "NicLink", "0 ns"), (d10, "nodeLink", "0 ns") )
dl11 = sst.Link("dl11")
dl11.connect( (n11, "NicLink", "0 ns"), (d11, "nodeLink", "0 ns") )
dl12 = sst.Link("dl12")
dl12.connect( (n12, "NicLink", "0 ns"), (d12, "nodeLink", "0 ns") )
dl13 = sst.Link("dl13")
dl13.connect( (n13, "NicLink", "0 ns"), (d13, "nodeLink", "0 ns") )
dl14 = sst.Link("dl14")
dl14.connect( (n14, "NicLink", "0 ns"), (d14, "nodeLink", "0 ns") )
dl15 = sst.Link("dl15")
dl15.connect( (n15, "NicLink", "0 ns"), (d15, "nodeLink", "0 ns") )
dl16 = sst.Link("dl16")
dl16.connect( (n16, "NicLink", "0 ns"), (d16, "nodeLink", "0 ns") )
dl17 = sst.Link("dl17")
dl17.connect( (n17, "NicLink", "0 ns"), (d17, "nodeLink", "0 ns") )
dl18 = sst.Link("dl18")
dl18.connect( (n18, "NicLink", "0 ns"), (d18, "nodeLink", "0 ns") )
dl19 = sst.Link("dl19")
dl19.connect( (n19, "NicLink", "0 ns"), (d19, "nodeLink", "0 ns") )
dl20 = sst.Link("dl20")
dl20.connect( (n20, "NicLink", "0 ns"), (d20, "nodeLink", "0 ns") )
dl21 = sst.Link("dl21")
dl21.connect( (n21, "NicLink", "0 ns"), (d21, "nodeLink", "0 ns") )
dl22 = sst.Link("dl22")
dl22.connect( (n22, "NicLink", "0 ns"), (d22, "nodeLink", "0 ns") )
dl23 = sst.Link("dl23")
dl23.connect( (n23, "NicLink", "0 ns"), (d23, "nodeLink", "0 ns") )
dl24 = sst.Link("dl24")
dl24.connect( (n24, "NicLink", "0 ns"), (d24, "nodeLink", "0 ns") )
dl25 = sst.Link("dl25")
dl25.connect( (n25, "NicLink", "0 ns"), (d25, "nodeLink", "0 ns") )
dl26 = sst.Link("dl26")
dl26.connect( (n26, "NicLink", "0 ns"), (d26, "nodeLink", "0 ns") )
dl27 = sst.Link("dl27")
dl27.connect( (n27, "NicLink", "0 ns"), (d27, "nodeLink", "0 ns") )
dl28 = sst.Link("dl28")
dl28.connect( (n28, "NicLink", "0 ns"), (d28, "nodeLink", "0 ns") )
dl29 = sst.Link("dl29")
dl29.connect( (n29, "NicLink", "0 ns"), (d29, "nodeLink", "0 ns") )
dl30 = sst.Link("dl30")
dl30.connect( (n30, "NicLink", "0 ns"), (d30, "nodeLink", "0 ns") )
dl31 = sst.Link("dl31")
dl31.connect( (n31, "NicLink", "0 ns"), (d31, "nodeLink", "0 ns") )
dl32 = sst.Link("dl32")
dl32.connect( (n32, "NicLink", "0 ns"), (d32, "nodeLink", "0 ns") )
dl33 = sst.Link("dl33")
dl33.connect( (n33, "NicLink", "0 ns"), (d33, "nodeLink", "0 ns") )
dl34 = sst.Link("dl34")
dl34.connect( (n34, "NicLink", "0 ns"), (d34, "nodeLink", "0 ns") )
dl35 = sst.Link("dl35")
dl35.connect( (n35, "NicLink", "0 ns"), (d35, "nodeLink", "0 ns") )
dl36 = sst.Link("dl36")
dl36.connect( (n36, "NicLink", "0 ns"), (d36, "nodeLink", "0 ns") )
dl37 = sst.Link("dl37")
dl37.connect( (n37, "NicLink", "0 ns"), (d37, "nodeLink", "0 ns") )
dl38 = sst.Link("dl38")
dl38.connect( (n38, "NicLink", "0 ns"), (d38, "nodeLink", "0 ns") )
dl39 = sst.Link("dl39")
dl39.connect( (n39, "NicLink", "0 ns"), (d39, "nodeLink", "0 ns") )
dl40 = sst.Link("dl40")
dl40.connect( (n40, "NicLink", "0 ns"), (d40, "nodeLink", "0 ns") )
dl41 = sst.Link("dl41")
dl41.connect( (n41, "NicLink", "0 ns"), (d41, "nodeLink", "0 ns") )
dl42 = sst.Link("dl42")
dl42.connect( (n42, "NicLink", "0 ns"), (d42, "nodeLink", "0 ns") )
dl43 = sst.Link("dl43")
dl43.connect( (n43, "NicLink", "0 ns"), (d43, "nodeLink", "0 ns") )
dl44 = sst.Link("dl44")
dl44.connect( (n44, "NicLink", "0 ns"), (d44, "nodeLink", "0 ns") )
dl45 = sst.Link("dl45")
dl45.connect( (n45, "NicLink", "0 ns"), (d45, "nodeLink", "0 ns") )
dl46 = sst.Link("dl46")
dl46.connect( (n46, "NicLink", "0 ns"), (d46, "nodeLink", "0 ns") )
dl47 = sst.Link("dl47")
dl47.connect( (n47, "NicLink", "0 ns"), (d47, "nodeLink", "0 ns") )
dl48 = sst.Link("dl48")
dl48.connect( (n48, "NicLink", "0 ns"), (d48, "nodeLink", "0 ns") )
dl49 = sst.Link("dl49")
dl49.connect( (n49, "NicLink", "0 ns"), (d49, "nodeLink", "0 ns") )
dl50 = sst.Link("dl50")
dl50.connect( (n50, "NicLink", "0 ns"), (d50, "nodeLink", "0 ns") )
dl51 = sst.Link("dl51")
dl51.connect( (n51, "NicLink", "0 ns"), (d51, "nodeLink", "0 ns") )
dl52 = sst.Link("dl52")
dl52.connect( (n52, "NicLink", "0 ns"), (d52, "nodeLink", "0 ns") )
dl53 = sst.Link("dl53")
dl53.connect( (n53, "NicLink", "0 ns"), (d53, "nodeLink", "0 ns") )
dl54 = sst.Link("dl54")
dl54.connect( (n54, "NicLink", "0 ns"), (d54, "nodeLink", "0 ns") )
dl55 = sst.Link("dl55")
dl55.connect( (n55, "NicLink", "0 ns"), (d55, "nodeLink", "0 ns") )
dl56 = sst.Link("dl56")
dl56.connect( (n56, "NicLink", "0 ns"), (d56, "nodeLink", "0 ns") )
dl57 = sst.Link("dl57")
dl57.connect( (n57, "NicLink", "0 ns"), (d57, "nodeLink", "0 ns") )
dl58 = sst.Link("dl58")
dl58.connect( (n58, "NicLink", "0 ns"), (d58, "nodeLink", "0 ns") )
dl59 = sst.Link("dl59")
dl59.connect( (n59, "NicLink", "0 ns"), (d59, "nodeLink", "0 ns") )
dl60 = sst.Link("dl60")
dl60.connect( (n60, "NicLink", "0 ns"), (d60, "nodeLink", "0 ns") )
dl61 = sst.Link("dl61")
dl61.connect( (n61, "NicLink", "0 ns"), (d61, "nodeLink", "0 ns") )
dl62 = sst.Link("dl62")
dl62.connect( (n62, "NicLink", "0 ns"), (d62, "nodeLink", "0 ns") )
dl63 = sst.Link("dl63")
dl63.connect( (n63, "NicLink", "0 ns"), (d63, "nodeLink", "0 ns") )
dl64 = sst.Link("dl64")
dl64.connect( (n64, "NicLink", "0 ns"), (d64, "nodeLink", "0 ns") )
dl65 = sst.Link("dl65")
dl65.connect( (n65, "NicLink", "0 ns"), (d65, "nodeLink", "0 ns") )
dl66 = sst.Link("dl66")
dl66.connect( (n66, "NicLink", "0 ns"), (d66, "nodeLink", "0 ns") )
dl67 = sst.Link("dl67")
dl67.connect( (n67, "NicLink", "0 ns"), (d67, "nodeLink", "0 ns") )
dl68 = sst.Link("dl68")
dl68.connect( (n68, "NicLink", "0 ns"), (d68, "nodeLink", "0 ns") )
dl69 = sst.Link("dl69")
dl69.connect( (n69, "NicLink", "0 ns"), (d69, "nodeLink", "0 ns") )
dl70 = sst.Link("dl70")
dl70.connect( (n70, "NicLink", "0 ns"), (d70, "nodeLink", "0 ns") )
dl71 = sst.Link("dl71")
dl71.connect( (n71, "NicLink", "0 ns"), (d71, "nodeLink", "0 ns") )
dl72 = sst.Link("dl72")
dl72.connect( (n72, "NicLink", "0 ns"), (d72, "nodeLink", "0 ns") )
dl73 = sst.Link("dl73")
dl73.connect( (n73, "NicLink", "0 ns"), (d73, "nodeLink", "0 ns") )
dl74 = sst.Link("dl74")
dl74.connect( (n74, "NicLink", "0 ns"), (d74, "nodeLink", "0 ns") )
dl75 = sst.Link("dl75")
dl75.connect( (n75, "NicLink", "0 ns"), (d75, "nodeLink", "0 ns") )
dl76 = sst.Link("dl76")
dl76.connect( (n76, "NicLink", "0 ns"), (d76, "nodeLink", "0 ns") )
dl77 = sst.Link("dl77")
dl77.connect( (n77, "NicLink", "0 ns"), (d77, "nodeLink", "0 ns") )
dl78 = sst.Link("dl78")
dl78.connect( (n78, "NicLink", "0 ns"), (d78, "nodeLink", "0 ns") )
dl79 = sst.Link("dl79")
dl79.connect( (n79, "NicLink", "0 ns"), (d79, "nodeLink", "0 ns") )

