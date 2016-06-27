#!/usr/bin/tclsh

gets stdin line
regexp {Total instruction count: ([0-9]*)} $line dummy totalInsns
#puts $totalInsns
puts $line

while {[gets stdin line]>=0} {
   if {[regexp {Instruction: (.*)} $line dummy instStr]} {
      #puts "inst: ($instStr)"
      gets stdin line
      regexp {Occurs: ([0-9]*) (.*)} $line dummy numOccurs rest
      #puts "occur ($numOccurs) rest ($rest)"
      puts "Instruction: [expr double($numOccurs) / $totalInsns] $instStr"
      puts $line
   } else {
      puts $line
   }
}

exit 0
