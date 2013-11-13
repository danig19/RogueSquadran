proc GetScenes {} {
  global snowmix
  puts $snowmix "tcl eval ScenesList"
  gets $snowmix line
  if {[string match "MSG: Invalid*" $line]} {
    tk_messageBox -title Alert -message "Snowmix has no scenes. Perhaps you forgot to include the library"
    return
  }
  if {![regexp {MSG:\ +Scenes\ +=\ +([0-9 ]+)} $line match scene_list]} {
    tk_messageBox -title Alert -message "tcl eval returned strange pattern <$line>"
    return
  }
  return $scene_list
}

proc GetImagesLoaded {} {
  global images snowmix
  set images(max_id) -1
  set image_list "-"
  puts $snowmix "image load"
  while {[gets $snowmix line] >= 0} {
    if {[string compare "MSG:" $line] == 0} break
    if {[regexp {MSG:\ +image\ +load\ +([0-9]+)\ +<([^ \>]+)>\ +([0-9]+)x([0-9]+)} $line match image_id filename width height]} {
      if {$image_id > $images(max_id)} { set images(max_id) $image_id }
      set images(width,$image_id) $width
      set images(height,$image_id) $height
      set images(name,$image_id) $filename
      lappend image_list "$image_id [file tail $filename]"
    }
  }
  set images(image_list) $image_list
  return
}

proc GetSceneInfo { scene_id } {
  global snowmix scene
  set lineno 0
  set scene(maxframe_id,$scene_id) -1
  puts $snowmix "tcl eval SceneList $scene_id"
  set scene(clocks,$scene_id) ""
  while {[gets $snowmix line] >= 0} {
    incr lineno
    if {[string compare "MSG: " $line] == 0} break
    if {[regexp {MSG:\ +Scene\ +[0-9]+\ +active\ +([0-9]+)\ +WxH\ +([0-9]+)x([0-9]+)\ +at\ +([0-9\.]+),([0-9\.]+)\ +name\ +(.+)} $line match active width height xoff yoff name]} {
      set scene(width,$scene_id) $width
      set scene(height,$scene_id) $height
      set scene(xoff,$scene_id) $xoff
      set scene(yoff,$scene_id) $yoff
      set scene(name,$scene_id) $name
      set scene(active,$scene_id) $active
      continue
    }
    if {[regexp {MSG:\ +\-\ +back\ +\:\ +image\ +([\-0-9])+\ +WxH\ +([0-9]+)x([0-9]+)\ +at\ +([0-9\.]+),([0-9\.]+)\ +shape\ +([0-9]+)\ +place\ +([0-9]+)} $line match image width height xoff yoff shape place]} {
      set scene(back,image,$scene_id) $image
      set scene(back,width,$scene_id) $width
      set scene(back,height,$scene_id) $height
      set scene(back,xoff,$scene_id) $xoff
      set scene(back,yoff,$scene_id) $yoff
      set scene(back,shape,$scene_id) $shape
      set scene(back,place,$scene_id) $place
      continue
    }
    if {[regexp {MSG:\ +\-\ +frame\ +([0-9])\ +active\ +([\-0-9]+)\ +\:\ +([0-9]+)x([0-9]+)\ +at\ +([0-9]+),([0-9]+)\ +source\ +([^ ,]+),([^ ,]+)\ +id\ +([\-0-9]+),([\-0-9]+)\ +shape\ +([0-9]+),([0-9]+)\ +place\ +([0-9]+),([0-9]+)} $line match frame_id active width height xoff yoff source_front source_back source_id_front source_id_back shape_front shape_back place_front place_back]} {
      set scene(active,$scene_id,$frame_id) $active
      set scene(width,$scene_id,$frame_id) $width
      set scene(height,$scene_id,$frame_id) $height
      set scene(xoff,$scene_id,$frame_id) $xoff
      set scene(yoff,$scene_id,$frame_id) $yoff
      set scene(source_front,$scene_id,$frame_id) "$source_front $source_id_front"
      set scene(source_back,$scene_id,$frame_id) "$source_back $source_id_back"
      set scene(shape_front,$scene_id,$frame_id) $shape_front
      set scene(shape_back,$scene_id,$frame_id) $shape_back
      set scene(place_front,$scene_id,$frame_id) $place_front
      set scene(place_back,$scene_id,$frame_id) $place_back
      if {$frame_id > $scene(maxframe_id,$scene_id)} { set scene(maxframe_id,$scene_id) $frame_id }
      continue
    }
    if {[regexp {MSG:\ +\-\ +clock\ +([0-9])\ +active\ +([\-0-9]+)\ +\:\ at\ +([0-9\.]+),([0-9\.]+)\ +text\ +([0-9\.]+)\ +([0-9\.]+)\ +([0-9\.]+)\ +([0-9\.]+)\ +bg\ +([0-9\.]+)\ +([0-9\.]+)\ +([0-9\.]+)\ +([0-9\.]+)} $line match clock_id active xoff yoff red green blue alpha bg_red bg_green bg_blue bg_alpha]} {
      lappend scene(clocks,$scene_id) $clock_id
      set scene(clock_active,$scene_id,$clock_id) $active
      set scene(clock_xoff,$scene_id,$clock_id) $xoff
      set scene(clock_yoff,$scene_id,$clock_id) $yoff
      set scene(clock_red,$scene_id,$clock_id) $red
      set scene(clock_green,$scene_id,$clock_id) $green
      set scene(clock_blue,$scene_id,$clock_id) $blue
      set scene(clock_alpha,$scene_id,$clock_id) $alpha
      set scene(clock_bg_red,$scene_id,$clock_id) $bg_red
      set scene(clock_bg_green,$scene_id,$clock_id) $bg_green
      set scene(clock_bg_blue,$scene_id,$clock_id) $bg_blue
      set scene(clock_bg_alpha,$scene_id,$clock_id) $bg_alpha
    }
  }
  return
}

proc SceneTabActive { scene_id active } {
  global scene
  if {[info exist scene(tab_id,$scene_id)]} {
    if {$active > 0} {
      set name ">> Scene $scene_id <<"
    } else {
      set name "Scene $scene_id"
    }
    set scene(active,$scene_id) $active
    $scene(pane) itemconfigure $scene(tab_id,$scene_id) -text $name
  }
  return
}

proc FrameActivate { scene_id frame_id fade } {
  global scene snowmix
  if {$scene(active,$scene_id,$frame_id) > 0} {
    if {$fade} {
      set command "tcl eval SetFrameFade $scene_id $frame_id 1\n"
    } else {
      set command "tcl eval SetFrameActive $scene_id $frame_id 0\n"
    }
    set scene(active,$scene_id,$frame_id) 0
    $scene(activatebutton,$scene_id,$frame_id) configure -bg red
  } else {
    if {$fade} {
      set command "tcl eval SetFrameFade $scene_id $frame_id 0\n"
    } else {
      set command "tcl eval SetFrameActive $scene_id $frame_id 1\n"
    }
    set scene(active,$scene_id,$frame_id) 1
    $scene(activatebutton,$scene_id,$frame_id) configure -bg green
  }
  puts $snowmix $command
  gets $snowmix line
puts $line
  return
}

proc FrameSourceSet { scene_id frame_id side } {
  global scene snowmix
  set source [$scene(combobox,source_$side,$scene_id,$frame_id) get]
  set scene(source_$side,$scene_id,$frame_id) $source
  if {[string match "-" $source]} {
    return
  }
  set source_type [lindex $source 0]
  if {[string match front $side]} {
    set front 1
    set active $scene(active,$scene_id,$frame_id)
  } else {
    set front 0
    set active 0
  }
  if {[string match feed $source_type] || [string match image $source_type] || [string match graph $source_type] || [string match radar $source_type]} {
    set id [lindex $source 1]
    set command "tcl eval SetSource4Frame $scene_id $frame_id $source_type $id $front $active"
    puts $snowmix $command
    gets $snowmix line
  } else {
    puts "Unsupported source type $source_type"
  }
}

proc SwopFrames { scene_id frame_id other_frame_id } {
  global scene
  set scene(source_back,$scene_id,$frame_id) $scene(source_front,$scene_id,$other_frame_id)
  set scene(source_back,$scene_id,$other_frame_id) $scene(source_front,$scene_id,$frame_id)

  SetComboBox $scene(combobox,source_back,$scene_id,$frame_id) $scene(source_back,$scene_id,$frame_id)
  SetComboBox $scene(combobox,source_back,$scene_id,$other_frame_id) $scene(source_back,$scene_id,$other_frame_id)
  FrameSourceSet $scene_id $frame_id back
  FrameSourceSet $scene_id $other_frame_id back
puts "Swopping scene $scene_id frame $frame_id to $scene(source_back,$scene_id,$frame_id) and frame $other_frame_id to $scene(source_back,$scene_id,$other_frame_id)"

  FrameFadeFrontBack $scene_id $frame_id 1
  FrameFadeFrontBack $scene_id $other_frame_id 1
  return
}
proc FrameFadeFrontBack { scene_id frame_id fade } {
  global scene snowmix
puts "FadeFront2Back for scene $scene_id frame $frame_id $scene(source_back,$scene_id,$frame_id) <-> $scene(shape_back,$scene_id,$frame_id)"
  set command "tcl eval Back2Front4Frame $scene_id $frame_id $fade\n"
  puts $snowmix $command
  gets $snowmix line
  set source $scene(source_front,$scene_id,$frame_id)
  set scene(source_front,$scene_id,$frame_id) $scene(source_back,$scene_id,$frame_id)
  set scene(source_back,$scene_id,$frame_id) $source
  set shape $scene(shape_front,$scene_id,$frame_id)
  set scene(shape_front,$scene_id,$frame_id) $scene(shape_back,$scene_id,$frame_id)
  set scene(shape_back,$scene_id,$frame_id) $shape
  set place $scene(place_front,$scene_id,$frame_id)
  set scene(place_front,$scene_id,$frame_id) $scene(place_back,$scene_id,$frame_id)
  set scene(place_back,$scene_id,$frame_id) $place
  SetComboBox $scene(combobox,source_front,$scene_id,$frame_id) $scene(source_front,$scene_id,$frame_id)
  SetComboBox $scene(combobox,source_back,$scene_id,$frame_id) $scene(source_back,$scene_id,$frame_id)
  return
}

proc ClockToggle { scene_id } {
  global scene snowmix
  foreach clock_id $scene(clocks,$scene_id) {
    #scene(clock_active,$scene_id,$clock_id)
    puts $snowmix "tcl eval TextClockToggle4Scene $scene_id $clock_id"
    gets $snowmix line
puts $line
  }
}
proc SceneActivate { scene_id } {
  global scene snowmix
  if {$scene(active,$scene_id) > 0} {
    $scene(activatebutton,$scene_id) configure -bg red
    set active 0
  } else {
    set active 1
    $scene(activatebutton,$scene_id) configure -bg green
  }
  foreach id $scene(scenes) {
    if {$scene(active,$id) > 0} {
      if {$id != $scene_id} {
        SceneTabActive $id 0
        $scene(activatebutton,$id) configure -bg red
      }
    }
  }
  SceneTabActive $scene_id $active
  puts $snowmix "tcl eval SetSceneState $scene_id $active"
  gets $snowmix line
  set scene(active,$scene_id) $active
}

proc CreateScenePage { scene_id } {
  global scene images snowmix
  if {![info exist scene(tab_id,$scene_id)]} return

  if {![info exist scene(feeds)]} {
    set scene(feed_list) "-"
    puts $snowmix "tcl eval ListFeedList"
    gets $snowmix line
    set scene(feeds) [lrange $line 3 end]
    foreach feed_id $scene(feeds) { lappend scene(feed_list) "feed $feed_id" }
#puts "feed list $scene(feed_list)\n"
  }
  if {![info exist scene(images)]} {
    set scene(image_list) ""
    foreach image $images(image_list) {
      lappend scene(images) [lindex $image 0]
      lappend scene(image_list) "image [lindex $image 0]"
    }
#puts "image list $scene(image_list)\n"
  }
  if {![info exist scene(graphs)]} {
    set scene(graph_list) ""
    puts $snowmix "tcl eval GraphsList"
    gets $snowmix line
    if {![string match "*Invalid*" $line]} {
      set scene(graphs) [lrange $line 3 end]
      foreach graph_id $scene(graphs) { lappend scene(graph_list) "graph $graph_id" }
#puts "graph list $scene(graph_list)\n"
    }
  }
  if {![info exist scene(radars)]} {
    set scene(radar_list) ""
    puts $snowmix "tcl eval RadarsList"
    gets $snowmix line
    if {![string match "*Invalid*" $line]} {
      set scene(radars) [lrange $line 3 end]
      foreach radar_id $scene(radars) { lappend scene(radar_list) "radar $radar_id" }
#puts "radar list $scene(radar_list)\n"
    }
  }
  set scenefr [$scene(pane) getframe $scene(tab_id,$scene_id)]
  set topfr [frame $scenefr.topfr]
  set midfr [frame $scenefr.midfr]
  set botfr [frame $scenefr.botfr]
  pack $topfr $midfr $botfr -side top -anchor w


  label $topfr.geometry -text "Geometry : $scene(width,$scene_id)x$scene(height,$scene_id)" \
    -padx 10 -relief sunken
  label $topfr.offset -text "Offset : $scene(xoff,$scene_id),$scene(yoff,$scene_id)" \
    -padx 10 -relief sunken
  label $topfr.name -text "Name : $scene(name,$scene_id)" -padx 10 -relief sunken
  #pack $topfr.fade -side left -fill x
#  if {[info exist scene(back,image,$scene_id)]} {
#    pack $topfr.bgimage -side left -fill x
#  }
  pack $topfr.geometry $topfr.offset $topfr.name -side left -fill x
      #set scene(active,$scene_id) $active

  set sfr [frame $botfr.scene$scene_id -padx 10 -pady 10 -borderwidth 2 -relief raised]
  pack $sfr -fill both -side left
# -anchor n

  # Scene activate Button
  if {$scene(active,$scene_id) > 0} { set bgcolor green
  } else { set bgcolor red }
  set scene(activatebutton,$scene_id) [Button $sfr.activate \
    -text "Scene $scene_id" -armcommand "SceneActivate $scene_id" \
    -bg $bgcolor -borderwidth 3]

  # Scene fade Button
  set scene(fadebutton,$scene_id) [Button $sfr.fade \
    -text "Scene Fade"  -armcommand "SceneFade $scene_id" -state disable]

  # Clock toggle
  set scene(clockbutton,$scene_id) [Button $sfr.clock \
    -text "ClockToggle"  -armcommand "ClockToggle $scene_id"]

  pack $sfr.activate $sfr.fade $sfr.clock -side top -fill x -anchor w

  # Scene background Selection
  if {[info exist scene(back,image,$scene_id)]} {
    Label $sfr.background -text "Background Image" -pady -1 -relief raised
    set scene(combobox,bgimage,$scene_id) [ComboBox $sfr.bgimage -values \
      $images(image_list) -width 30 -modifycmd "BackgroundImageChange $scene_id"]
    SetComboBox $sfr.bgimage "$scene(back,image,$scene_id) *"
    pack $sfr.background $sfr.bgimage -side top -fill x -anchor w
  }
  set cfr [frame $sfr.cfr  -pady 4]
  pack $cfr -side top -fill x -anchor w
  set canvas [canvas $cfr.canvas -bg black \
    -width [expr $scene(width,$scene_id) / 5] \
    -height [expr $scene(height,$scene_id) / 5]]
  pack $canvas -side top -fill x -anchor w
  set frame_id 0
  while { $frame_id <= $scene(maxframe_id,$scene_id) } {
    if {![info exist scene(width,$scene_id,$frame_id)]} {
      incr frame_id
      continue
    }
    set scene(canvas,scale,$scene_id,$frame_id) 5.0
    set scale $scene(canvas,scale,$scene_id,$frame_id)
    set xoff [expr $scene(xoff,$scene_id,$frame_id) / $scale]
    set yoff [expr $scene(yoff,$scene_id,$frame_id) / $scale]
    set x2 [expr ($scene(width,$scene_id,$frame_id) + $scene(xoff,$scene_id,$frame_id)) / $scale]
    set y2 [expr ($scene(height,$scene_id,$frame_id) + $scene(yoff,$scene_id,$frame_id)) / $scale]
    $canvas create rect $xoff $yoff $x2 $y2 -outline grey -fill lightblue -width 1 -tag frame_$frame_id
    set x [expr ($xoff + $x2)/2]
    set y [expr ($yoff + $y2)/2]
    set scene(canvas,$scene_id,$frame_id) $canvas
    set scene(canvaspress,$scene_id,$frame_id) 0
    $canvas create text $x $y -text "Frame $frame_id" -fill black -tag frame_$frame_id
    $canvas bind frame_$frame_id <ButtonPress> "scene_canvas_press $scene_id $frame_id %b press %x %y"
    $canvas bind frame_$frame_id <ButtonRelease> "scene_canvas_press $scene_id $frame_id %b release %x %y"
    $canvas bind frame_$frame_id <Motion> "scene_canvas_press $scene_id $frame_id %b motion %x %y"
    incr frame_id
  }
  if {[info exist scene(clocks,$scene_id)]} {
    set scale 5.0
    foreach clock_id $scene(clocks,$scene_id) {
      set x [expr $scene(clock_xoff,$scene_id,$clock_id) / $scale -40]
      set y [expr $scene(clock_yoff,$scene_id,$clock_id) / $scale -4]
      set x2 [expr $x + 70]
      set y2 [expr $y + 14]
      $canvas create rect $x $y $x2 $y2 -outline grey -fill lightblue -width 1 -tag clock_$clock_id
    }
  }

  set frame_id 0
  while { $frame_id <= $scene(maxframe_id,$scene_id) } {
    if {![info exist scene(width,$scene_id,$frame_id)]} {
      incr frame_id
      continue
    }
    set fr [frame $botfr.frame$frame_id -padx 5 -pady 8 -borderwidth 2 -relief raised ]
    pack $fr -side left -fill both -anchor w

    # Frame Active Button
    if {$scene(active,$scene_id,$frame_id) > 0} { set bgcolor green
    } else { set bgcolor red }
    set scene(activatebutton,$scene_id,$frame_id) [Button $fr.name \
      -text "Frame $frame_id" -armcommand "FrameActivate $scene_id $frame_id 0" \
      -bg $bgcolor -borderwidth 3]
    set scene(fadebutton,$scene_id,$frame_id) [Button $fr.fade \
      -text "Frame Fade" -armcommand "FrameActivate $scene_id $frame_id 1"]

    # Frame Swop Button
    set scene(swopbutton,$scene_id,$frame_id) [Button $fr.swop \
      -text "Swop Front/Back" -armcommand "FrameFadeFrontBack $scene_id $frame_id 0"]
    # Frame Fade Button
    set scene(swopfadebutton,$scene_id,$frame_id) [Button $fr.swopfade \
      -text "Fade Front/Back" -armcommand "FrameFadeFrontBack $scene_id $frame_id 1"]

    # Frames Swop Button
    set swfr [frame $fr.swfr]
    Label $swfr.label -text "Swop Frames" -relief raised
    pack $swfr.label -side left -fill x -anchor w
    set other_frame_id 0
    while { $other_frame_id <= $scene(maxframe_id,$scene_id) } {
      if {![info exist scene(width,$scene_id,$other_frame_id)] || $other_frame_id == $frame_id} {
        incr other_frame_id
        continue
      }
      Button $swfr.swop$other_frame_id -text "$other_frame_id" -armcommand "SwopFrames $scene_id $frame_id $other_frame_id" -pady -1
      pack $swfr.swop$other_frame_id -side left -fill x -anchor w
      incr other_frame_id
    }

    Label $fr.front -text "Front Source" -relief raised
    Label $fr.back -text "Back Source" -relief raised
    pack $fr.name $fr.fade $fr.swop $fr.swopfade $swfr -side top -fill x -anchor w
    set side_list [list front back]
    foreach side $side_list {
      set ffs [frame $fr.f$side -pady 2]
      pack $fr.$side $ffs -side top -fill x -anchor w
      set ffsl [frame $ffs.ffsl]
      set ffsr [frame $ffs.ffsr]
      pack $ffsl -side left -fill both -anchor w
      pack $ffsr -side right -fill both 
      Label $ffsl.source -text "Source" -relief raised -width 7 -pady 3
      Label $ffsl.shape  -text "Pla/sha" -relief raised
      Label $ffsl.place  -text "Coors" -relief raised
      pack $ffsl.source $ffsl.shape $ffsl.place -side top -fill x -anchor w
      set scene(combobox,source_$side,$scene_id,$frame_id) \
        [ComboBox $ffsr.source_$side -values "$scene(feed_list) $scene(image_list) $scene(graph_list) $scene(radar_list)" \
        -width 9 -justify right -modifycmd "FrameSourceSet $scene_id $frame_id $side"]
      SetComboBox $ffsr.source_$side $scene(source_$side,$scene_id,$frame_id)

      frame $ffsr.shape_$side
      Label $ffsr.shape_$side.place  -relief sunken -anchor e -textvariable \
        scene(place_$side,$scene_id,$frame_id) -width 5
      Label $ffsr.shape_$side.shape  -relief sunken -anchor e -textvariable \
        scene(shape_$side,$scene_id,$frame_id) -width 5
      pack $ffsr.shape_$side.place $ffsr.shape_$side.shape -side left -fill x -anchor w

      frame $ffsr.coor_$side
      Label $ffsr.coor_$side.x -relief sunken -anchor e -text 0 -width 5
      Label $ffsr.coor_$side.y -relief sunken -anchor e -text 0 -width 5
      pack $ffsr.coor_$side.x $ffsr.coor_$side.y -side left -fill x -anchor w

      pack $ffsr.source_$side $ffsr.shape_$side $ffsr.coor_$side -side top -fill x -anchor e
    }
    Label $fr.frame -text "Frame" -relief raised
    pack $fr.frame -side top -fill x -anchor w

    set frl [frame $fr.left]
    set frr [frame $fr.right]
    pack $frl $frr -side left -fill x -anchor w
    label $frl.geometry     -text Geometry       -relief raised -anchor w -width 10
    label $frl.offset       -text Offset         -relief raised -anchor w
    pack $frl.geometry $frl.offset -side top -anchor w -fill x
    label $frr.geometry     -relief sunken -anchor e -width 8 -text \
      "$scene(width,$scene_id,$frame_id)x$scene(height,$scene_id,$frame_id)"
    label $frr.offset       -relief sunken -anchor e -text \
      "$scene(xoff,$scene_id,$frame_id),$scene(yoff,$scene_id,$frame_id)"

    pack $frr.geometry $frr.offset -side top -anchor w -fill x
    incr frame_id
  }

}

proc SetComboBox { cbox value } {
  set entry_id 0
  foreach entry [$cbox cget -values] {
    if {[string match $value $entry]} {
      $cbox setvalue @$entry_id
      break
    }
    incr entry_id
  }
}

proc BackgroundImageChange {scene_id} {
  global scene snowmix
  set image_id [lindex [$scene(combobox,bgimage,$scene_id) get] 0]
  puts $snowmix "tcl eval Setbackground4Scene $scene_id $image_id"
  gets $snowmix line
  puts $line
  return
}

proc scene_canvas_press {scene_id frame_id but type x y } {
  global scene snowmix
  if {$scene(canvaspress,$scene_id,$frame_id) && [string match motion $type]} {
    set dx [expr ($x - $scene(canvas,motion_x,$scene_id,$frame_id))/5.0]
    set dy [expr ($y - $scene(canvas,motion_y,$scene_id,$frame_id))/5.0]
    if {$dx != 0 || $dy != 0} {
      $scene(canvas,$scene_id,$frame_id) move frame_$frame_id $dx $dy
      set coors [$scene(canvas,$scene_id,$frame_id) coords frame_$frame_id]
      set xoff [lindex $coors 0]
      set yoff [lindex $coors 1]
      set x2 [lindex $coors 2]
      set y2 [lindex $coors 3]
      set width [expr round(5.0 * ($x2 - $xoff))]
      set height [expr round(5.0 * ($y2 - $yoff))]
      set xoff [expr round(5.0 * $xoff)]
      set yoff [expr round(5.0 * $yoff)]
      puts "offset $xoff $yoff $width $height"
      set command "tcl eval PlaceFrame4Scene $scene_id $frame_id $xoff $yoff $width $height"
      puts $snowmix $command
      gets $snowmix line
      puts $line
    }
    set scene(canvas,motion_x,$scene_id,$frame_id) $x
    set scene(canvas,motion_y,$scene_id,$frame_id) $y
  } elseif {[string match press $type] && $but == 1} {
    set scene(canvaspress,$scene_id,$frame_id) 1
    set scene(canvas,motion_x,$scene_id,$frame_id) $x
    set scene(canvas,motion_y,$scene_id,$frame_id) $y
  puts "$type scene $scene_id frame $frame_id but $but x $x y $y"
  } elseif {[string match release $type] && $but == 1} {
    set scene(canvaspress,$scene_id,$frame_id) 0
  puts "$type scene $scene_id frame $frame_id but $but x $x y $y"
  }
  return
}

proc scene_pane { pane } {
  global snowmix scene
  set scene(scenes) [GetScenes]
  GetImagesLoaded
  puts "Scenes are : $scene(scenes)"

  ScrollableFrame $pane.scrollframe -constrainedwidth false -constrainedheight false \
    -xscrollcommand "$pane.scrollbar set" -width 1200 -height 408
  scrollbar $pane.scrollbar -command "$pane.scrollframe xview" -orient horizontal
  pack $pane.scrollframe -side top
  pack $pane.scrollbar -side top -fill x
  set panefr [$pane.scrollframe getframe]

  set sp [NoteBook $panefr.nb -side top -arcradius 4 -homogeneous 1 -tabbevelsize 2]
  set scene(pane) $sp
  pack $sp -side top -fill both -anchor w
  foreach scene_id $scene(scenes) {
    GetSceneInfo $scene_id
    if {[info exist scene(name,$scene_id)]} {
      set scene(tab_id,$scene_id) scene$scene_id
      $sp insert end scene$scene_id
      # $sp insert end scene$scene_id -text $name
      SceneTabActive $scene_id $scene(active,$scene_id)
      CreateScenePage $scene_id
      #set scene_pane [$sp getframe $scene(tab_id,$scene_id)]
      if {$scene(active,$scene_id) > 0} { $sp raise scene$scene_id }
    }
  }
}

