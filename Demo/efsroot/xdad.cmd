
anim1 0 {
   spr0 4
   delay 3
   spr0 5
   delay 3
   spr0 6
   delay 3
   spr0 7
   delay 3
   spr0 8
   delay 3
   spr0 9
   delay 3
   spr0 a
   delay 3
   spr0 b
   delay 3
   loop
}

state0 :anim1 {
    using dpad
    using momentum(x to 512)
    using momentum(y to 512)
}

end
