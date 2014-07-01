# scheduler simulation input file
import sst

# Define SST core options
sst.setProgramOption("run-mode", "both")
sst.setProgramOption("partitioner", "self")

# Define the simulation components
scheduler = sst.Component("myScheduler", "scheduler.schedComponent")
scheduler.addParams({
      "traceName" : "test_scheduler_0003.sim", # job trace path (required)
      "scheduler" : "easy", # cons, delayed, easy, elc, pqueue, prioritize. (default: pqueue)
      "machine" : "mesh[4,5,2]", # mesh[xdim, ydim, zdim], simple. (default: simple)
      "allocator" : "energy", # bestfit, constraint, energy, firstfit, genalg,
                              # granularmbs, hybrid, mbs, mc1x1, mm, nearest,
                              # octetmbs, oldmc1x1,random, simple, sortedfreelist.
                              # (default: simple)
      "taskMapper" : "simple", # simple, rcb, random (default: simple)
      "FST" : "none", # none, relaxed, strict. (default: none)
      "timeperdistance" : ".7[.3,0.9875,0.0962,.2]", # communication overhead params (default: none)
      "runningTimeSeed" : "42", # communication overhead randomization seed (default: none)
      "dMatrixFile" : "DMatrix4_5_2" # heat recirculation matrix path (default: none)
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

