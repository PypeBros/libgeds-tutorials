# -*- sh -*-
# simple demo script. With no maps yet, there isn't much to demonstrate ^^"
#   
print "loading level1"
bg0.load "../bg.spr"
print "loading sprites"
spr.load "../hero.spr":1

print "declaring state machine"

input "xdad.cmd"
import state 0
print "creating gobs"
gob0 :state0 (128, 100) h # rightmost
focus=gob0
print "go!"

end
