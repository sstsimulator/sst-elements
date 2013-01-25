#!/usr/bin/tclsh

#
# Program to test whether integer generation achieves 
# the necessary real-valued average. It does, try it.
# - target average needs supplied as command line arg
#

set avg [lindex $argv 0]

set total 0
set count 0
for {set i 0} {$i < 10000} {incr i} {
   # rand() produces a value between 0 and 1
   # 6 is the range of possible values, so avg +/- 3
   set r [expr round((rand()*5-2.5)+$avg)]
   puts $r
   incr count 
   incr total $r
}
puts "target average: $avg"
puts "average: [expr double($total)/$count]"

exit 0

