import sst

obj = sst.Component("simpleExternalElement", "simpleExternalElement.SimpleExternalElement")
obj.addParams({
    "printFrequency" : "5",
    "repeats" : "15"
    })

