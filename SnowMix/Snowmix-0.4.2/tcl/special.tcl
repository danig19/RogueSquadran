proc special_pane { pane } {
  global counter_val

  set fr [frame $pane.special]
  pack $fr -side top -anchor w

  set hide [frame $fr.hide -relief raised -borderwidth 2]
  set show [frame $fr.show -relief raised -borderwidth 2]
  set cover [frame $fr.cover -relief raised -borderwidth 2]
  set counter [frame $fr.counter -relief raised -borderwidth 2]
  set mix [frame $fr.mix -relief raised -borderwidth 2]
  set canvas [canvas $fr.canvas -bg black -width 512 -height 288]
  pack $hide $show $cover $counter $mix $canvas -side left -anchor n -padx 4

  set bwidth 2
  label $hide.text -text Hide
  Button $hide.logo   -text Logo   -borderwidth $bwidth \
    -armcommand "SwitchState $show.logo $hide.logo ; HideInSnowmix logo"
  Button $hide.roller -text Roller -borderwidth $bwidth \
    -armcommand "SwitchState $show.roller $hide.roller ; HideInSnowmix roller"
  Button $hide.date   -text Date   -borderwidth $bwidth \
    -armcommand "SwitchState $show.date $hide.date ; HideInSnowmix date"
  Button $hide.timing -text Timing -borderwidth $bwidth \
    -armcommand "SwitchState $show.timing $hide.timing ; HideInSnowmix timing"
  Button $hide.numbers -text Numbers -borderwidth $bwidth \
    -armcommand "SwitchState $show.numbers $hide.numbers ; HideInSnowmix counter ; HideInSnowmix alcohol; HideInSnowmix lox; HideInSnowmix chamber"
  Button $hide.counter -text Counter -borderwidth $bwidth \
    -armcommand "SwitchState $show.counter $hide.counter ; HideInSnowmix counter"
  Button $hide.alcohol -text Alcohol -borderwidth $bwidth \
    -armcommand "SwitchState $show.alcohol $hide.alcohol ; HideInSnowmix alcohol"
  Button $hide.lox -text Lox -borderwidth $bwidth \
    -armcommand "SwitchState $show.lox $hide.lox ; HideInSnowmix lox"
  Button $hide.chamber -text Chamber -borderwidth $bwidth \
    -armcommand "SwitchState $show.chamber $hide.chamber ; HideInSnowmix chamber"
  Button $hide.head1   -text Head1   -borderwidth $bwidth \
    -armcommand "SwitchState $show.head1 $hide.head1 ; HideInSnowmix head1"
  Button $hide.head2   -text Head2   -borderwidth $bwidth \
    -armcommand "SwitchState $show.head2 $hide.head2 ; HideInSnowmix head2"
  Button $hide.head3   -text Head3   -borderwidth $bwidth \
    -armcommand "SwitchState $show.head3 $hide.head3 ; HideInSnowmix head3"
  Button $hide.head4   -text Head4   -borderwidth $bwidth \
    -armcommand "SwitchState $show.head4 $hide.head4 ; HideInSnowmix head4"
  pack $hide.text $hide.logo $hide.roller $hide.date $hide.timing $hide.numbers $hide.counter $hide.alcohol $hide.lox $hide.chamber $hide.head1 $hide.head2 $hide.head3 $hide.head4 -side top -fill x

  label $show.text -text Show
  Button $show.logo   -text Logo   -borderwidth $bwidth -state disabled \
    -armcommand "SwitchState $hide.logo $show.logo ; ShowInSnowmix logo"
  Button $show.roller -text Roller -borderwidth $bwidth -state disabled \
    -armcommand "SwitchState $hide.roller $show.roller ; ShowInSnowmix roller"
  Button $show.date   -text Date   -borderwidth $bwidth -state disabled \
    -armcommand "SwitchState $hide.date $show.date ; ShowInSnowmix date"
  Button $show.timing -text Timing -borderwidth $bwidth -state disabled \
    -armcommand "SwitchState $hide.timing $show.timing ; ShowInSnowmix timing"
  Button $show.numbers -text Numbers -borderwidth $bwidth \
    -armcommand "SwitchState $hide.numbers $show.numbers ; ShowInSnowmix counter ; ShowInSnowmix alcohol; ShowInSnowmix lox; ShowInSnowmix chamber"
  Button $show.counter -text Counter -borderwidth $bwidth -state disabled \
    -armcommand "SwitchState $hide.counter $show.counter ; ShowInSnowmix counter"
  Button $show.alcohol -text Alcohol -borderwidth $bwidth -state disabled \
    -armcommand "SwitchState $hide.alcohol $show.alcohol ; ShowInSnowmix alcohol"
  Button $show.lox -text Lox -borderwidth $bwidth -state disabled \
    -armcommand "SwitchState $hide.lox $show.lox ; ShowInSnowmix lox"
  Button $show.chamber -text Chamber -borderwidth $bwidth -state disabled \
    -armcommand "SwitchState $hide.chamber $show.chamber ; ShowInSnowmix chamber"
  Button $show.head1   -text Head1   -borderwidth $bwidth -state disabled \
    -armcommand "SwitchState $hide.head1 $show.head1 ; ShowInSnowmix head1"
  Button $show.head2   -text Head2   -borderwidth $bwidth -state disabled \
    -armcommand "SwitchState $hide.head2 $show.head2 ; ShowInSnowmix head2"
  Button $show.head3   -text Head3   -borderwidth $bwidth -state disabled \
    -armcommand "SwitchState $hide.head3 $show.head3 ; ShowInSnowmix head3"
  Button $show.head4   -text Head4   -borderwidth $bwidth -state disabled \
    -armcommand "SwitchState $hide.head4 $show.head4 ; ShowInSnowmix head4"
  pack $show.text $show.logo $show.roller $show.date $show.timing $show.numbers $show.counter $show.alcohol $show.lox $show.chamber $show.head1 $show.head2 $show.head3 $show.head4 -side top -fill x

  label $cover.text1 -text Cover
  Button $cover.small1   -text "Small 1"   -borderwidth $bwidth \
    -armcommand "SwitchState $cover.small1u $cover.small1 ; CoverInSnowmix 0 small1"
  Button $cover.small2   -text "Small 2"   -borderwidth $bwidth \
    -armcommand "SwitchState $cover.small2u $cover.small2 ; CoverInSnowmix 0 small2"
  Button $cover.small3   -text "Small 3"   -borderwidth $bwidth \
    -armcommand "SwitchState $cover.small3u $cover.small3 ; CoverInSnowmix 0 small3"
  Button $cover.small1u  -text "Small 1"   -borderwidth $bwidth -state disabled \
    -armcommand "SwitchState $cover.small1 $cover.small1u ; CoverInSnowmix 1 small1"
  Button $cover.small2u  -text "Small 2"   -borderwidth $bwidth -state disabled \
    -armcommand "SwitchState $cover.small2 $cover.small2u ; CoverInSnowmix 1 small2"
  Button $cover.small3u  -text "Small 3"   -borderwidth $bwidth -state disabled \
    -armcommand "SwitchState $cover.small3 $cover.small3u ; CoverInSnowmix 1 small3"
  label $cover.text2 -text Uncover
  pack $cover.text1 $cover.small1 $cover.small2 $cover.small3 $cover.text2 $cover.small1u $cover.small2u $cover.small3u -side top -fill x

  set counter_val(hours) 0
  set counter_val(minutes) 15
  set counter_val(seconds) 0
  label $counter.label -text Counter
  frame $counter.value 
  SpinBox $counter.value.hours   -range "0 99 1" -width 2 -textvariable counter_val(hours)
  SpinBox $counter.value.minutes -range "0 59 1" -width 2 -textvariable counter_val(minutes)
  SpinBox $counter.value.seconds -range "0 59 1" -width 2 -textvariable counter_val(seconds)
  pack $counter.value.hours $counter.value.minutes $counter.value.seconds -side left -fill x
  Button $counter.set   -text Set -armcommand "CounterSet set"
  Button $counter.start -text Start -state normal -armcommand \
    "SwitchState $counter.stop $counter.start; CounterSet start"
  Button $counter.stop  -text Stop -state disabled -armcommand \
    "SwitchState $counter.start $counter.stop; CounterSet stop"
  Button $counter.up    -text Up -armcommand "CounterSet up"
  Button $counter.down  -text Down -armcommand "CounterSet down"
  Button $counter.position1  -text "Small 1" -armcommand "CounterSet pos1"
  Button $counter.position2  -text "Small 2" -armcommand "CounterSet pos2"
  Button $counter.position3  -text "Small 3" -armcommand "CounterSet pos3"
  pack $counter.label $counter.value $counter.set $counter.start $counter.stop $counter.up $counter.down $counter.position1 $counter.position2 $counter.position3 -side top -fill x

  label $mix.label -text Mix
  Button $mix.m4123 -text 4-321 -state normal -armcommand "Mix 4123 $mix"
  Button $mix.m1423 -text 1-423 -state normal -armcommand "Mix 1423 $mix"
  Button $mix.m2143 -text 2-143 -state normal -armcommand "Mix 2143 $mix"
  Button $mix.m3124 -text 3-124 -state normal -armcommand "Mix 3124 $mix"
  Button $mix.right -text "Move Right" -state normal -armcommand "Mix right $mix"
  Button $mix.left -text "Move Left" -state disabled -armcommand "Mix left $mix"
  Button $mix.swop1 -text "Swop 1-4" -state normal -armcommand "Mix swop14 $mix"
  Button $mix.swop2 -text "Swop 2-4" -state normal -armcommand "Mix swop24 $mix"
  Button $mix.swop3 -text "Swop 3-4" -state normal -armcommand "Mix swop34 $mix"
  pack $mix.label $mix.m4123 $mix.m1423 $mix.m2143 $mix.m3124 $mix.right $mix.left $mix.swop1 $mix.swop2 $mix.swop3 -side top -fill x
  Mix 4123 $mix
}


proc Mix { mix buttonroot } {
  global snowmix main_mix
  if {[string match 4123 $mix]} {
    set cmd "feed4123" ; set main_mix 4123
    $buttonroot.right configure -state normal
    $buttonroot.left configure -state disabled
    $buttonroot.swop1 configure -state normal
    $buttonroot.swop2 configure -state normal
    $buttonroot.swop3 configure -state normal
  } elseif {[string match 1423 $mix]} {
    set cmd "feed1423" ; set main_mix 1423
    $buttonroot.right configure -state normal
    $buttonroot.left configure -state disabled
    $buttonroot.swop1 configure -state normal
    $buttonroot.swop2 configure -state disabled
    $buttonroot.swop3 configure -state disabled
  } elseif {[string match 2143 $mix]} {
    set cmd "feed2143" ; set main_mix 2143
    $buttonroot.right configure -state normal
    $buttonroot.left configure -state disabled
    $buttonroot.swop1 configure -state disabled
    $buttonroot.swop2 configure -state normal
    $buttonroot.swop3 configure -state disabled
  } elseif {[string match 3124 $mix]} {
    set cmd "feed3124" ; set main_mix 3124
    $buttonroot.right configure -state normal
    $buttonroot.left configure -state disabled
    $buttonroot.swop1 configure -state disabled
    $buttonroot.swop2 configure -state disabled
    $buttonroot.swop3 configure -state normal
  } elseif {[string match right $mix]} {
    if {[string match 4123 $main_mix]} {
      set cmd "right123"
      set main_mix 4
    } elseif {[string match 1423 $main_mix]} {
      set cmd "right423"
      set main_mix 1
    } elseif {[string match 2143 $main_mix]} {
      set cmd "right143"
      set main_mix 2
    } elseif {[string match 3124 $main_mix]} {
      set cmd "right124"
      set main_mix 3
    } else return
    $buttonroot.swop1 configure -state disabled
    $buttonroot.swop2 configure -state disabled
    $buttonroot.swop3 configure -state disabled
    $buttonroot.left configure -state normal
    $buttonroot.right configure -state disabled
  } elseif {[string match left $mix]} {
    if {[string match 4 $main_mix]} {
      set cmd "left123"
      set main_mix 4123
      $buttonroot.swop1 configure -state normal
      $buttonroot.swop2 configure -state normal
      $buttonroot.swop3 configure -state normal
    } elseif {[string match 1 $main_mix]} {
      set cmd "left423"
      set main_mix 1423
      $buttonroot.swop1 configure -state normal
      $buttonroot.swop2 configure -state disabled
      $buttonroot.swop3 configure -state disabled
    } elseif {[string match 2 $main_mix]} {
      set cmd "left143"
      set main_mix 2143
    } elseif {[string match 3 $main_mix]} {
      set cmd "left124"
      set main_mix 3124
      $buttonroot.swop1 configure -state disabled
      $buttonroot.swop2 configure -state disabled
      $buttonroot.swop3 configure -state normal
    } else return
    $buttonroot.right configure -state normal
    $buttonroot.left configure -state disabled
  } elseif {[string match "swop??" $mix]} {

    if {[string match "swop14" $mix]} {
      if {[string match "4123" $main_mix]} {
        set cmd "swop41"
        set main_mix 1423
        $buttonroot.swop2 configure -state disabled
        $buttonroot.swop3 configure -state disabled
      } elseif {[string match "1423" $main_mix]} {
        set cmd "swop14"
        set main_mix 4123
        $buttonroot.swop2 configure -state normal
        $buttonroot.swop3 configure -state normal
      } else return
      $buttonroot.swop1 configure -state normal
    } elseif {[string match "swop24" $mix]} {
      if {[string match "4123" $main_mix]} {
        set cmd "swop42"
        set main_mix 2143
        $buttonroot.swop1 configure -state disabled
        $buttonroot.swop3 configure -state disabled
      } elseif {[string match "2143" $main_mix]} {
        set cmd "swop24"
        set main_mix 4123
        $buttonroot.swop1 configure -state normal
        $buttonroot.swop3 configure -state normal
      } else return
      $buttonroot.swop2 configure -state normal
    } elseif {[string match "swop34" $mix]} {
      if {[string match "4123" $main_mix]} {
        set cmd "swop43"
        set main_mix 3124
        $buttonroot.swop1 configure -state disabled
        $buttonroot.swop2 configure -state disabled
      } elseif {[string match "3124" $main_mix]} {
        set cmd "swop34"
        set main_mix 4123
        $buttonroot.swop1 configure -state normal
        $buttonroot.swop2 configure -state normal
      } else return
      $buttonroot.swop3 configure -state normal
    } else return
  } else return
  puts $snowmix $cmd
}

proc HideInSnowmix { element } {
  global snowmix
  if {[string match roller $element]} {
    set cmd "text place backgr move clip 5 0 0 0.04 0 0 0 25 0"
  } elseif {[string match logo $element]} {
    set cmd "image place alpha 0 0"
  } elseif {[string match date $element]} {
    set cmd "text place backgr move clip 20 0 0 0.04 0 0 0 25 0"
  } elseif {[string match timing $element]} {
    set cmd "text place backgr move clip 6 0 0 0.04 0 0 0 25 0\n
     text place backgr move clip 7 0 0 0.04 0 0 0 25 0\n
     text place backgr move clip 8 0 0 0.04 0 0 0 25 0\n
     text place backgr move clip 9 0 0 0.04 0 0 0 25 0"
  } elseif {[string match head1 $element]} {
    set cmd "text place backgr move clip 1 0 0 0.04 0 0 0 25 0"
  } elseif {[string match head2 $element]} {
    set cmd "text place backgr move clip 2 0 0 0.04 0 0 0 25 0"
  } elseif {[string match head3 $element]} {
    set cmd "text place backgr move clip 3 0 0 0.04 0 0 0 25 0"
  } elseif {[string match head4 $element]} {
    set cmd "text place backgr move clip 4 0 0 0.04 0 0 0 25 0"
  } elseif {[string match counter $element]} {
    set cmd "image place move alpha 21 -0.02 50\n
      image place move alpha 22 -0.02 50\n
      image place move alpha 23 -0.02 50\n
      image place move alpha 24 -0.02 50\n
      image place move alpha 25 -0.02 50\n
      image place move alpha 26 -0.02 50\n
      image place move alpha 27 -0.02 50\n
      image place move alpha 28 -0.02 50
      text place move alpha 27 -0.02 50\n"
  } elseif {[string match alcohol $element]} {
    set cmd "image place move alpha 30 -0.02 50\n
      image place move alpha 31 -0.02 50\n
      image place move alpha 32 -0.02 50\n
      image place move alpha 33 -0.02 50\n
      text place move alpha 21 -0.02 50\n
      text place move alpha 24 -0.02 50\n"
  } elseif {[string match lox $element]} {
    set cmd "image place move alpha 34 -0.02 50\n
      image place move alpha 35 -0.02 50\n
      image place move alpha 36 -0.02 50\n
      image place move alpha 37 -0.02 50\n
      text place move alpha 22 -0.02 50\n
      text place move alpha 25 -0.02 50\n"
  } elseif {[string match chamber $element]} {
    set cmd "image place move alpha 38 -0.02 50\n
      image place move alpha 39 -0.02 50\n
      image place move alpha 40 -0.02 50\n
      image place move alpha 41 -0.02 50\n
      text place move alpha 23 -0.02 50\n
      text place move alpha 26 -0.02 50\n"
  } else return
  puts $snowmix $cmd
}

proc ShowInSnowmix { element } {
  global snowmix
  if {[string match roller $element]} {
    set cmd "text place backgr move clip 5 0 0 -0.04 0 0 0 25 0"
  } elseif {[string match logo $element]} {
    set cmd "image place alpha 0 1"
  } elseif {[string match date $element]} {
    set cmd "text place backgr move clip 20 0 0 -0.04 0 0 0 25 0"
  } elseif {[string match timing $element]} {
    set cmd "text place backgr move clip 6 0 0 -0.04 0 0 0 25 0\n
     text place backgr move clip 7 0 0 -0.04 0 0 0 25 0\n
     text place backgr move clip 8 0 0 -0.04 0 0 0 25 0\n
     text place backgr move clip 9 0 0 -0.04 0 0 0 25 0"
  } elseif {[string match head1 $element]} {
    set cmd "text place backgr move clip 1 0 0 -0.04 0 0 0 25 0"
  } elseif {[string match head2 $element]} {
    set cmd "text place backgr move clip 2 0 0 -0.04 0 0 0 25 0"
  } elseif {[string match head3 $element]} {
    set cmd "text place backgr move clip 3 0 0 -0.04 0 0 0 25 0"
  } elseif {[string match head4 $element]} {
    set cmd "text place backgr move clip 4 0 0 -0.04 0 0 0 25 0"
  } elseif {[string match counter $element]} {
    set cmd "image place move alpha 21 0.02 50\n
      image place move alpha 22 0.02 50\n
      image place move alpha 23 0.02 50\n
      image place move alpha 24 0.02 50\n
      image place move alpha 25 0.02 50\n
      image place move alpha 26 0.02 50\n
      image place move alpha 27 0.02 50\n
      image place move alpha 28 0.02 50
      text place move alpha 27 0.02 50\n"
  } elseif {[string match alcohol $element]} {
    set cmd "image place move alpha 30 0.02 50\n
      image place move alpha 31 0.02 50\n
      image place move alpha 32 0.02 50\n
      image place move alpha 33 0.02 50\n
      text place move alpha 21 0.02 50\n
      text place move alpha 24 0.02 50\n"
  } elseif {[string match lox $element]} {
    set cmd "image place move alpha 34 0.02 50\n
      image place move alpha 35 0.02 50\n
      image place move alpha 36 0.02 50\n
      image place move alpha 37 0.02 50\n
      text place move alpha 22 0.02 50\n
      text place move alpha 25 0.02 50\n"
  } elseif {[string match chamber $element]} {
    set cmd "image place move alpha 38 0.02 50\n
      image place move alpha 39 0.02 50\n
      image place move alpha 40 0.02 50\n
      image place move alpha 41 0.02 50\n
      text place move alpha 23 0.02 50\n
      text place move alpha 26 0.02 50\n"
  } else return
  puts $snowmix $cmd
}
proc CoverInSnowmix { cover element } {
  global snowmix
  if {$cover == 0} {set cover "0.02" } else {set cover "-0.02"}
  if {[string match small1 $element]} {
    set cmd "image place move alpha 1 $cover 50"
  } elseif {[string match small2 $element]} {
    set cmd "image place move alpha 2 $cover 50"
  } elseif {[string match small3 $element]} {
    set cmd "image place move alpha 3 $cover 50"
  } else return
  puts $snowmix $cmd
}

proc CounterSet {action} {
  global snowmix counter_val
  if {[string match set $action]} {
    set cmd "command pop DigiCounterTcl 10
        command push DigiCounterTcl set a \[SetDigiCounter $counter_val(hours) \
         $counter_val(minutes) $counter_val(seconds)\]
        command push DigiCounterTcl return \[format \"\\n%s\" \$a]
        tcl exec DigiCounterTcl"
  } elseif {[string match start $action]} {
    set cmd "command pop DigiCounterSeconds_Switch 10
      command push DigiCounterSeconds_Switch DigiCounterSeconds
      command push DigiCounterSeconds_Switch loop"
  } elseif {[string match stop $action]} {
    set cmd "command pop DigiCounterSeconds_Switch 10"
  } elseif {[string match up $action]} {
    set cmd "image place move coor 21 0 -4 0 48
      image place move coor 22 0 -4 0 48
      image place move coor 23 0 -4 0 48
      image place move coor 24 0 -4 0 48
      image place move coor 25 0 -4 0 48
      image place move coor 26 0 -4 0 48
      image place move coor 27 0 -4 0 48
      image place move coor 28 0 -4 0 48
      image place move coor 30 0 -4 0 48
      image place move coor 31 0 -4 0 48
      image place move coor 32 0 -4 0 48
      image place move coor 33 0 -4 0 48
      image place move coor 34 0 -4 0 48
      image place move coor 35 0 -4 0 48
      image place move coor 36 0 -4 0 48
      image place move coor 37 0 -4 0 48
      image place move coor 38 0 -4 0 48
      image place move coor 39 0 -4 0 48
      image place move coor 40 0 -4 0 48
      image place move coor 41 0 -4 0 48
      text place move coor 21 0 -4 0 48
      text place move coor 22 0 -4 0 48
      text place move coor 23 0 -4 0 48
      text place move coor 24 0 -4 0 48
      text place move coor 25 0 -4 0 48
      text place move coor 26 0 -4 0 48
      text place move coor 27 0 -4 0 48"
  } elseif {[string match down $action]} {
    set cmd "image place move coor 21 0 4 0 48
      image place move coor 22 0 4 0 48
      image place move coor 23 0 4 0 48
      image place move coor 24 0 4 0 48
      image place move coor 25 0 4 0 48
      image place move coor 26 0 4 0 48
      image place move coor 27 0 4 0 48
      image place move coor 28 0 4 0 48
      image place move coor 30 0 4 0 48
      image place move coor 31 0 4 0 48
      image place move coor 32 0 4 0 48
      image place move coor 33 0 4 0 48
      image place move coor 34 0 4 0 48
      image place move coor 35 0 4 0 48
      image place move coor 36 0 4 0 48
      image place move coor 37 0 4 0 48
      image place move coor 38 0 4 0 48
      image place move coor 39 0 4 0 48
      image place move coor 40 0 4 0 48
      image place move coor 41 0 4 0 48
      text place move coor 21 0 4 0 48
      text place move coor 22 0 4 0 48
      text place move coor 23 0 4 0 48
      text place move coor 24 0 4 0 48
      text place move coor 25 0 4 0 48
      text place move coor 26 0 4 0 48
      text place move coor 27 0 4 0 48"
  } elseif {[string match "pos?" $action]} {
    if {[string match pos1 $action]} {
      set y 136
    } elseif {[string match pos2 $action]} {
      set y 328
    } else { set y 520 }
    set x 794
    set xt [expr $x + 41]
    set yt [expr $y - 40]
    set xbar [expr $x - 64]
    set ybar [expr $y - 90]
    set xbart [expr $x - 39]
    set ybart [expr $y - 130]
    set cmd "image place coor 21 [expr $x + 142] $y
      image place coor 22 [expr $x + 118] $y
      image place coor 23 [expr $x + 101] $y
      image place coor 24 [expr $x + 84] $y
      image place coor 25 [expr $x + 59] $y
      image place coor 26 [expr $x + 42] $y
      image place coor 27 [expr $x + 25] $y
      image place coor 28 [expr $x+ 0] $y
      image place coor 30 [expr $xbar+ 0] $ybar
      image place coor 31 [expr $xbar+ 25] $ybar
      image place coor 32 [expr $xbar+ 42] $ybar
      image place coor 33 [expr $xbar+ 59] $ybar
      image place coor 34 [expr $xbar+ 105] $ybar
      image place coor 35 [expr $xbar+ 130] $ybar
      image place coor 36 [expr $xbar+ 147] $ybar
      image place coor 37 [expr $xbar+ 164] $ybar
      image place coor 38 [expr $xbar+ 210] $ybar
      image place coor 39 [expr $xbar+ 235] $ybar
      image place coor 40 [expr $xbar+ 252] $ybar
      image place coor 41 [expr $xbar+ 269] $ybar
      text place coor 21 $xbart $ybart
      text place coor 22 [expr $xbart + 110] $ybart
      text place coor 23 [expr $xbart + 220] $ybart
      text place coor 24 $xbart [expr $ybart + 65]
      text place coor 25 [expr $xbart + 110] [expr $ybart + 65]
      text place coor 26 [expr $xbart + 220] [expr $ybart + 65]
      text place coor 27 $xt $yt"
  } else return
  puts $snowmix "$cmd"
}

proc SwitchState { enabled disabled } {
  $enabled configure -state normal
  $disabled configure -state disabled
}

set main_mix 4123
