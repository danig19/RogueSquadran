proc videoclip_pane { pane } {
  global video_shape unit_shape alpha_value

  set video_shape(button) place
  set video_shape(x) 0
  set video_shape(y) 0
  set video_shape(pressed) 0
  GetShapes
  if { $video_shape(shape_max) < 0 } { return }
  GetPatterns
  GetPlacedShapes

  set width 512
  set height 288

  # Frame for pane
  set fr [frame $pane.clip]
  pack $fr -side top -anchor w

  set sfr [frame $fr.selectfr]
  set ifr [frame $fr.inputfr]
  pack $sfr $ifr -side left -fill x

  label $sfr.label -text "Placed Shape" -relief raised -pady 5
  set bfr [frame $sfr.bfr]
  pack $sfr.label $bfr -side top -fill x

  set btfr [frame $bfr.top]
  set bbfr [frame $bfr.bottom]
  pack $btfr $bbfr -side top -fill x

  set lfr [frame $btfr.lfr]
  set rfr [frame $btfr.rfr]
  pack $lfr $rfr -side left

  label $lfr.placelabel -text "Placed Id" -relief raised -pady 3
  label $lfr.shapelabel -text "Shape Id " -relief raised -pady 3
  label $lfr.sourcelabel -text "Source Id" -relief raised -pady 3 -justify left
  label $lfr.cliplabel -text "Clip Id     " -relief raised -pady 3 -justify left
  label $lfr.patternlabel -text "Pattern Id" -relief raised -pady 3 -justify left
  pack $lfr.placelabel $lfr.shapelabel $lfr.sourcelabel $lfr.cliplabel \
    $lfr.patternlabel -fill x -side top -anchor w

  # Combobox for Placed Shapes
  set video_shape(combo_placed_shape) [ComboBox $rfr.placeshape \
    -values [GetListOfPlacedShapes] -modifycmd PlacedShapeChange \
    -width 3 -justify right]
  $rfr.placeshape setvalue first
  #set video_shape(combo_placed_shape) $rfr.placeshape

  # Combobox for shape of placed shapes
  set video_shape(combo_shape_id) [ComboBox $rfr.shapeid \
    -values [GetListOfShapes] -modifycmd ShapeChange -width 16 \
    -justify right]

  # Combobox for source of shape of shape of placed shape
  set video_shape(combo_source_id) [ComboBox $rfr.source \
    -values [GetListShapePattern "Source *"] -width 16 -justify right]
  $video_shape(combo_source_id) setvalue first

  # Combobox for Clips
  set video_shape(combo_clip_id) [ComboBox $rfr.clip -values \
    [GetListShapePattern "Unit *"] -width 16 -justify right \
    -modifycmd ClipShapeChange]
  $video_shape(combo_clip_id) setvalue first

  # Combobox for pattern
  set pattern_list "-"
  set id 0
  while {$id <= $video_shape(pattern_max)} {
    if {[info exist video_shape(pattern_name,$id)]} {
      lappend pattern_list "$id $video_shape(pattern_name,$id)"
    }
    incr id
  }
  set video_shape(combo_pattern_id) [ComboBox $rfr.pattern \
    -values $pattern_list -width 16 -justify right]
  $video_shape(combo_pattern_id) setvalue first

  pack $rfr.placeshape $rfr.shapeid $rfr.source $rfr.clip $rfr.pattern \
    -side top -anchor e

  PlacedShapeChange
  #set video_shape(combo_source) $rfr.source
  set video_shape(place_shape) [$rfr.placeshape get]
  get_placed_shape $video_shape(place_shape)

  label $bbfr.secondlabel -text "Placed Shape Details" -relief raised -pady 4
  frame $bbfr.holder
  pack $bbfr.secondlabel $bbfr.holder -side top -fill x
  set holder_l [frame $bbfr.holder.left]
  set holder_r [frame $bbfr.holder.right]
  pack $holder_l -side left
  pack $holder_r -side right

  label $holder_l.x        -text "x pos   " -relief raised -anchor w -width 6
  label $holder_l.y        -text "y pos   " -relief raised -anchor w
  label $holder_l.xoffset  -text "x offset" -relief raised -anchor w
  label $holder_l.yoffset  -text "y offset" -relief raised -anchor w
  label $holder_l.scalex   -text "x scale " -relief raised -anchor w
  label $holder_l.scaley   -text "y scale " -relief raised -anchor w
  label $holder_l.rotation -text "rotation" -relief raised -anchor w
  label $holder_l.alpha    -text "alpha"    -relief raised -anchor w
  label $holder_l.sourcex  -text "x source" -relief raised -anchor w
  label $holder_l.sourcey  -text "y source" -relief raised -anchor w
  label $holder_l.sourcesx -text "sx source" -relief raised -anchor w
  label $holder_l.sourcesy -text "sy source" -relief raised -anchor w
  pack $holder_l.x $holder_l.y $holder_l.xoffset $holder_l.yoffset \
    $holder_l.scalex $holder_l.scaley $holder_l.rotation $holder_l.alpha \
    $holder_l.sourcex $holder_l.sourcey $holder_l.sourcesx $holder_l.sourcesy \
    -side top -fill x
  bind $holder_l.rotation <ButtonPress> "reset_placed_shape rotation"
  bind $holder_l.alpha <ButtonPress> "reset_placed_shape alpha"
  bind $holder_l.xoffset <ButtonPress> "reset_placed_shape offset"
  bind $holder_l.yoffset <ButtonPress> "reset_placed_shape offset"

  label $holder_r.x        -textvariable video_shape(place_x) -relief sunken -anchor e \
    -width 15
  label $holder_r.y        -textvariable video_shape(place_y)      -relief sunken -anchor e
  label $holder_r.xoffset  -textvariable video_shape(place_offx)   -relief sunken -anchor e
  label $holder_r.yoffset  -textvariable video_shape(place_offy)   -relief sunken -anchor e
  label $holder_r.scalex   -textvariable video_shape(place_scalex) -relief sunken -anchor e
  label $holder_r.scaley   -textvariable video_shape(place_scaley) -relief sunken -anchor e
  label $holder_r.rotation -textvariable video_shape(place_rotate) -relief sunken -anchor e
  label $holder_r.alpha    -textvariable video_shape(place_alpha)  -relief sunken -anchor e
  label $holder_r.sourcex  -textvariable video_shape(source_offx)  -relief sunken -anchor e
  label $holder_r.sourcey  -textvariable video_shape(source_offy)  -relief sunken -anchor e
  label $holder_r.sourcesx -textvariable video_shape(source_scalex)  -relief sunken -anchor e
  label $holder_r.sourcesy -textvariable video_shape(source_scaley)  -relief sunken -anchor e
  pack $holder_r.x $holder_r.y $holder_r.xoffset $holder_r.yoffset \
    $holder_r.scalex $holder_r.scaley $holder_r.rotation $holder_r.alpha \
    $holder_r.sourcex $holder_r.sourcey $holder_r.sourcesx $holder_r.sourcesy \
    -side top -fill x

  set buttonfr [frame $ifr.buttons]
  Button $buttonfr.place  -text Place \
    -armcommand "clip_active_button $buttonfr place"
  Button $buttonfr.clip   -text "Clip offset" \
    -armcommand "clip_active_button $buttonfr clip"
  Button $buttonfr.rotate -text Rotate \
    -armcommand "clip_active_button $buttonfr rotate"
  Button $buttonfr.zoom   -text "Clip zoom" \
    -armcommand "clip_active_button $buttonfr zoom"
  Button $buttonfr.size   -text Size \
    -armcommand "clip_active_button $buttonfr size"
  Button $buttonfr.offset  -text Offset \
    -armcommand "clip_active_button $buttonfr offset"
  Button $buttonfr.freehand  -text Freehand \
    -armcommand "clip_active_button $buttonfr freehand"
  clip_active_button $buttonfr place

  pack $buttonfr.place $buttonfr.offset $buttonfr.size $buttonfr.rotate \
    $buttonfr.freehand $buttonfr.zoom $buttonfr.clip -side left

  frame $ifr.cv
  set canvas [canvas $ifr.cv.canvas -bg black -width $width -height $height]
  pack $buttonfr $ifr.cv -side top -fill x
  set alpha_value [expr sqrt(10000 * $video_shape(place_alpha))]
  set slide [scale $ifr.cv.scale -from 100 -to 0 -length 288 \
    -orient vertical -variable alpha_value -showval 0 -width 10 \
    -command "AlphaChange"]
  pack $canvas $slide -side left
  set video_shape(canvas) $canvas

  bind $canvas <ButtonPress> "canvas_press 1 %b %x %y"
  bind $canvas <ButtonRelease> "canvas_press 0 %b 0 0"
  bind $canvas <Motion> "canvas_motion %x %y"
}

# Function used to reset rotation alpha and offset of a placed shape
proc reset_placed_shape { type } {
  global snowmix video_shape alpha_value
  set command ""
  if {[string match rotation $type]} {
    set command "shape place rotation $video_shape(place_id) 0"
  } elseif {[string match alpha $type]} {
    set command "shape place alpha $video_shape(place_id) 1.0"
    set alpha_value 100
  } elseif {[string match offset $type]} {
    set command "shape place offset $video_shape(place_id) 0.0 0.0"
  } else {
    return
  }
  puts $snowmix $command
  gets $snowmix line
  get_placed_shape $video_shape(place_shape)
}

proc AlphaChange {value} {
  global snowmix video_shape
  #set shape [PlacedShape]
  set video_shape(place_alpha) [expr ($value * $value)/10000.0]
  set command "shape place alpha $video_shape(place_shape) $video_shape(place_alpha)"
  puts $snowmix $command
  gets $snowmix line
}

# Function to call when the combobox changes the placed shape
proc PlacedShapeChange {} {
  global video_shape alpha_value
  set video_shape(place_shape) [$video_shape(combo_placed_shape) get]
  get_placed_shape $video_shape(place_shape)
  SetShapeBox $video_shape(shape_id,$video_shape(place_shape))
  set alpha_value [expr sqrt(10000*$video_shape(place_alpha,$video_shape(place_shape)))]
}

# Function to call when the combobox changes the shape of the placed shape
proc ShapeChange {} {
  global snowmix video_shape
  set video_shape(shape_id) [lindex [$video_shape(combo_shape_id) get] 0]
  set command "shape place shape $video_shape(place_shape) $video_shape(shape_id)"
  puts $command
  puts $snowmix $command
  gets $snowmix line
  get_placed_shape $video_shape(place_shape)
  set $video_shape(shape_id,$video_shape(place_shape)) $video_shape(shape_id)
  SetShapeBox $video_shape(shape_id)
}

# Function to set the shape id for the shape id combobox
proc SetShapeBox { shape_id } {
  global video_shape
  set n 0
  foreach entry [$video_shape(combo_shape_id) cget -values] {
    if {[string match "$shape_id *" $entry]} {
      $video_shape(combo_shape_id) setvalue @$n
      break
    }
    incr n
  }
  GetShapeElements $shape_id
  set clip_shape [GetClipShape $shape_id]
  set source_shape [GetSourceShape $shape_id]
  SetComboShapeBox $clip_shape $video_shape(combo_clip_id)
  SetComboShapeBox $source_shape $video_shape(combo_source_id)
  SetComboShapeBox [GetPatternId $shape_id] $video_shape(combo_pattern_id)
  if {$clip_shape > -1} {
  }
  SetSourceSettings $source_shape
}

proc ClipShapeChange {} {
  global snowmix video_shape
  set shape_id [lindex [$video_shape(combo_shape_id) get] 0]
  set clip_id [lindex [$video_shape(combo_clip_id) get] 0]
  puts "Clip Change for $shape_id to shape $clip_id"
  if {[string match "-" $clip_id]} {
     # No clip. Maybe we delete clip in future
    return
  }
  puts $snowmix "shape inshape $shape_id $clip_id\nshape moveentry $shape_id first\nshape moveentry $shape_id last first"
  gets $snowmix line
  gets $snowmix line
  GetShapeElements $shape_id
}

proc SetComboShapeBox { shape_id combobox } {
  global video_shape
  $combobox setvalue @0
  if {$shape_id > -1} {
    foreach entry [$combobox cget -values] {
      if {[string match "$shape_id *" $entry]} {
        $combobox setvalue @$n
        break
      }
      incr n
    }
  }
}

# get the clip shape id of shape shape_id
proc GetClipShape { shape_id } {
  global video_shape
  set inshape -1
  set clipshape -1
  set shapeline -1
  set n 0
  foreach command $video_shape(shape_command,$shape_id) {
    incr n
    if {[string match "clip" $command]} {
      set clipshape $inshape
      break
    }
    if {[regexp {inshape\ +([0-9]+)} $command match shape]} {
      set inshape $shape
      set shapeline $n
    }
  }
  set video_shape(clipshape,$shape_id) $clipshape
  set video_shape(clipshapeline,$shape_id) $shapeline
  return $clipshape
}

# Function to get source type, offset and scale
proc SetSourceSettings { shape_id } {
puts "SOURCE SETTINGS"
  global snowmix video_shape
  set video_shape(source_type) ""
  set video_shape(source_type_id) ""
  set video_shape(source_offx) ""
  set video_shape(source_offy) ""
  set video_shape(source_scalex) ""
  set video_shape(source_scaley) ""
  if {$shape_id < 0} return
  set line_no 0
  puts $snowmix "shape list $shape_id"
  while {[gets $snowmix line] >= 0} {
    incr line_no
puts "GOT $line"
    if {[string compare "STAT: " $line] == 0} break
    if {[regexp {STAT:\ +- [0-9]+ +([^ ]+)\ +([0-9]+)\ +at\ +([\-0-9\.]+),([\-0-9\.]+)\ +scale\ +([\-0-9\.]+),([\-0-9\.]+)} $line match type id x y scalex scaley]} {
      set video_shape(source_type) $type
      set video_shape(source_type_id) $id
      set video_shape(source_offx) [format "%.0f" $x]
      set video_shape(source_offy) [format "%.0f" $y]
      set video_shape(source_scalex) $scalex
      set video_shape(source_scaley) $scaley
    }
  }
}

# get the source shape id of shape shape_id
proc GetSourceShape { shape_id } {
  global video_shape
  set inshape -1
  set sourceshape -1
  foreach command $video_shape(shape_command,$shape_id) {
    if {[string match "clip" $command]} {
      set sourceshape -1
      set inshape -1
    } elseif {[string match "paint *" $command]} {
      set sourceshape $inshape
    } elseif {[string match "mask pattern *" $command]} {
      set sourceshape $inshape
    } elseif {[string match "inshape *" $command]} {
      set inshape [lindex $command 1]
    }
  }
  return $sourceshape
}

# get the pattern id of shape shape_id
proc GetPatternId { shape_id } {
  global video_shape
  set pattern_id -1
  foreach command $video_shape(shape_command,$shape_id) {
    if {[string match "mask pattern *" $command]} {
      set pattern_id [lindex $command 2]
puts "PMM $command = $pattern_id"
    } elseif {[string match "clip" $command]} {
      set pattern_id -1
    }
  }
  return $pattern_id
}


# Get the shape elements of shape <shape_id>
# and make them listed in video_shape(shape_command,shape_id)
proc GetShapeElements {shape_id} {
  global snowmix video_shape
  set video_shape(shape_command,$shape_id) ""
  puts $snowmix "shape list $shape_id"
  while {[gets $snowmix line] >= 0} {
    if {[string compare "STAT: " $line] == 0} break
    if {[regexp {STAT:\ +- ([0-9]+)\ +(.+)} $line match shape_line shape_command]} {
      lappend video_shape(shape_command,$shape_id) $shape_command
    }
  }
  puts $video_shape(shape_command,$shape_id)
  foreach command $video_shape(shape_command,$shape_id) {
    puts " - : <$command>"
  }
}

proc GetListShapePattern { pattern } {
  global video_shape
  set shape_list "-"
  set id 0
  while {$id <= $video_shape(shape_max)} {
    if {[info exist video_shape(shape,$id)]} {
      if {[string match $pattern $video_shape(shape,$id)]} {
        lappend shape_list "$id $video_shape(shape,$id)"
      }
    }
    incr id
  }
  return $shape_list
}

# Getting shape pattern ids and names.
# Setting names in video_shape(pattern,id)
# Setting number type in video_shape(pattern_type,id)
# Setting max shape id in video_shape(pattern_max)
proc GetPatterns {} {
  global snowmix video_shape
  puts $snowmix "shape pattern add"
  set pattern_id -1
  while {[gets $snowmix line] >= 0} {
puts "GOT $line"
    if {[string compare "STAT: " $line] == 0} break
    # STAT:  shape pattern 1 type linear stops 2 name Unit Linear Top-Bottom
    if {[regexp {STAT:\ +shape\ +pattern\ +([0-9]+)\ +type\ +([^ ]+) +stops\ +([0-9]+)\ +name\ +(.+)} \
      $line match pattern_id type stops name]} {
      set video_shape(pattern,$pattern_id) $name
      set video_shape(pattern_type,$pattern_id) $type
      set video_shape(pattern_stops,$pattern_id) $stops
      set video_shape(pattern_name,$pattern_id) $name
#      puts $video_shape(shape,$shape_id)
    }
  }
  set video_shape(pattern_max) $pattern_id
}

# Getting shape ids and names.
# Setting names in video_shape(shape,id)
# Setting number ops in video_shape(shape_ops,id)
# Setting max shape id in video_shape(shape_max)
proc GetShapes {} {
  global snowmix video_shape
  puts $snowmix "shape add"
  set shape_id -1
  while {[gets $snowmix line] >= 0} {
    if {[string compare "STAT: " $line] == 0} break
    if {[regexp {STAT:\ +shape\ +([0-9]+)\ +ops\ +([0-9]+) +name\ +(.+)} \
      $line match shape_id ops name]} {
      set video_shape(shape,$shape_id) $name
      set video_shape(shape_ops,$shape_id) $ops
#      puts $video_shape(shape,$shape_id)
    }
  }
  set video_shape(shape_max) $shape_id
}

proc GetPlacedShapes {} {
  global snowmix video_shape
  puts $snowmix "shape place"
  set place_id -1
  while {[gets $snowmix line] >= 0} {
    if {[string compare "STAT: " $line] == 0} break
    if {[regexp {STAT:\ +shape\ place\ +([0-9]+)\ +shape\ +([0-9]+)\ +at\ +([\-\.0-9]+),([\-\.0-9]+)\ +off\ +([\-\.0-9]+),([\-\.0-9]+)\ +rot\ +([\-\.0-9]+)\ +scale\ +([\-\.0-9]+),([\-\.0-9]+)\ +rgb\ +([\.0-9]+),([\.0-9]+),([\.0-9]+)\ +alpha\ +([\.0-9]+)} $line match place_id shape_id x y offx offy rot scalex scaley red green blue alpha]} {
      set video_shape(place_id) $place_id
      set video_shape(shape_id,$place_id) $shape_id
      set video_shape(place_x,$place_id) $x
      set video_shape(place_y,$place_id) $y
      set video_shape(place_offx,$place_id) $offx
      set video_shape(place_offy,$place_id) $offy
      set video_shape(place_rotate,$place_id) $rot
      set video_shape(place_scalex,$place_id) $scalex
      set video_shape(place_scaley,$place_id) $scaley
      set video_shape(place_alpha,$place_id) $alpha
      continue
    } else {
      puts "no match for $line"
    }
  }
  set video_shape(shape_place_max) $place_id
}

# Get settings for placed shape and save settings for active placed shape
proc get_placed_shape { shape } {
  global snowmix video_shape
  puts $snowmix "shape place"
  while {[gets $snowmix line] >= 0} {
    if {[string compare "STAT: " $line] == 0} break
    if {[regexp {STAT:\ +shape\ place\ +([0-9]+)\ +shape\ +([0-9]+)\ +at\ +([\-\.0-9]+),([\-\.0-9]+)\ +off\ +([\-\.0-9]+),([\-\.0-9]+)\ +rot\ +([\-\.0-9]+)\ +scale\ +([\-\.0-9]+),([\-\.0-9]+)\ +rgb\ +([\.0-9]+),([\.0-9]+),([\.0-9]+)\ +alpha\ +([\.0-9]+)} $line match place_id shape_id x y offx offy rot scalex scaley red green blue alpha]} {
      if {$place_id == $shape} {
        puts "match $place_id $x $y"
        set video_shape(place_id) $place_id
        set video_shape(shape_id) $shape_id
        set video_shape(place_x) $x
        set video_shape(place_y) $y
        set video_shape(place_offx) $offx
        set video_shape(place_offy) $offy
        set video_shape(place_rotate) $rot
        set video_shape(place_scalex) $scalex
        set video_shape(place_scaley) $scaley
        set video_shape(place_alpha) $alpha
      }
      continue
    } else {
      puts "no match for $line"
    }
  }
}


#
# get a list of shapes
# STAT:  shape place 1 shape 21 at 652.000,288.000 off 0.0000,0.0000 rot 0.000 scale 100.000,100.000 rgb 0.000,0.000,0.000 alpha 1.000
proc GetListOfPlacedShapes {} {
  global snowmix
  set shape_list ""
  puts $snowmix "shape place"
  while {[gets $snowmix line] >= 0} {
    if {[string compare "STAT: " $line] == 0} break
    if {[regexp {STAT:\ +shape\ +place +([0-9]+)\ +shape\ +([0-9]+)} $line match place_id shape_id]} {
      lappend shape_list $place_id
      set video_shape(place_shape,$place_id) $shape_id
    }
  }
  return $shape_list
}

proc GetListOfShapes {} {
  global video_shape
  set shape_list ""
  set id 0
  while {$id <= $video_shape(shape_max) } {
    if {[info exist  video_shape(shape,$id)]} {
      if {[string match "Unit *" $video_shape(shape,$id)] || [string match "Feed Shape *" $video_shape(shape,$id)]} {
        incr id
        continue
      }
      lappend shape_list "$id $video_shape(shape,$id)"
    }
    incr id
  }
  return $shape_list
}
# set video_shape(shape,$shape_id) $name
# set video_shape(shape_ops,$shape_id) $ops
# set video_shape(shape_max) $shape_id

proc clip_active_button { buttonfr type } {
  global video_shape
  foreach name { place clip rotate zoom size offset freehand } {
    $buttonfr.$name configure -state normal -activebackground grey \
    -background lightgrey
  }
  $buttonfr.$type configure -state active -activebackground green \
    -background grey
  set video_shape(active) $type
}

proc ProcessFreehandList { shape freehand_list } {
  global snowmix video_shape
  set minx 0
  set maxx 0
  set miny 0
  set maxy 0
  set x 0
  set y 0
  foreach coor $freehand_list {
    set delta_x [lindex $coor 0]
    set delta_y [lindex $coor 1]
    set x [expr $x + $delta_x]
    set y [expr $y + $delta_y]
    if {$x > $maxx} { set maxx $x }
    if {$y > $maxy} { set maxy $y }
    if {$x < $minx} { set minx $x }
    if {$y < $miny} { set miny $y }
  }
  set scale_x [expr $maxx - $minx]
  set scale_y [expr $maxy - $miny]
  if { $scale_x > $scale_y } { set scale $scale_x
  } else { set scale $scale_y }
  set add_x [expr ($maxx + $minx) / 2.0]
  set add_y [expr ($maxy + $miny) / 2.0]
  set x [expr $add_x / $scale]
  set y [expr $add_y / $scale]
  puts $snowmix "shape add $shape\nshape add $shape Unit Freehand"
  gets $snowmix line
  gets $snowmix line
  puts $snowmix "shape moveto $shape $x $y"
  gets $snowmix line
  foreach coor $freehand_list {
    set delta_x [lindex $coor 0]
    set delta_y [lindex $coor 1]
    set x [expr (0.0 + $delta_x) / $scale]
    set y [expr (0.0 + $delta_y) / $scale]
    puts $snowmix "shape linerel $shape $x $y"
    gets $snowmix line
  }
  puts $snowmix "shape closepath $shape $x $y"
  $video_shape(canvas) delete freehand
}


proc canvas_press { press button x y} {
  global video_shape
  if { $button != 1} return
  if { $press == 0 } {
puts "Release $video_shape(active)"
    set video_shape(pressed) 0
    if {[string match freehand $video_shape(active)]} {
      ProcessFreehandList 9 $video_shape(freehand_list)
    }
    return
  }
  set video_shape(pressed) 1
  set video_shape(x) $x
  set video_shape(y) $y
  set video_shape(freehand_list) ""
  puts "Button pressed was $button $x $y"
  get_placed_shape $video_shape(place_shape)
  #lappend video_shape(freehand_list) "$x $y"
}

proc ShapePlaceChange { delta_x delta_y } {
  global video_shape snowmix
  set video_shape(place_x) [expr $video_shape(place_x) + $delta_x]
  set video_shape(place_y) [expr $video_shape(place_y) + $delta_y]
  set command "shape place coor $video_shape(place_id) $video_shape(place_x) $video_shape(place_y)"
  puts $snowmix $command
  gets $snowmix line
}

proc ShapePlaceOffsetChange { delta_x delta_y } {
  global video_shape snowmix
  set video_shape(place_offx) [expr $video_shape(place_offx) + $delta_x]
  set video_shape(place_offy) [expr $video_shape(place_offy) + $delta_y]
  set command "shape place offset $video_shape(place_id) $video_shape(place_offx) $video_shape(place_offy)"
  puts $snowmix $command
  gets $snowmix line
}
proc ShapePlaceSizeChange { delta_x delta_y } {
  global video_shape snowmix
  set scalex [expr $video_shape(place_scalex) + $delta_x]
  set scaley [expr $video_shape(place_scaley) - $delta_y]
  if { $scalex > 0 } { set video_shape(place_scalex) $scalex }
  if { $scaley > 0 } { set video_shape(place_scaley) $scaley }
  set command "shape place scale $video_shape(place_id) $video_shape(place_scalex) $video_shape(place_scaley)"
  puts $snowmix $command
  gets $snowmix line
}

proc ShapePlaceRotationChange { delta_x delta_y } {
  global video_shape snowmix
  set delta_rotate 0
  if {$delta_x < 0} {
    set delta_rotate -0.025
  } elseif {$delta_x > 0} {
    set delta_rotate 0.025
  }
  if {$delta_rotate != 0} {
    set video_shape(place_rotate) [format "%.4f" [expr $video_shape(place_rotate) + $delta_rotate]]
    set command "shape place rotation $video_shape(place_id) $video_shape(place_rotate)"
    puts $snowmix $command
    gets $snowmix line
  }
}

proc canvas_motion { x y } {
  global snowmix video_shape
  if { $video_shape(pressed) == 0 } return
  set delta_x [expr $x - $video_shape(x)]
  set delta_y [expr $y - $video_shape(y)]
  #puts "mouse position $x $y motion $delta_x $delta_y"
  if {[string match place $video_shape(active)]} {
    ShapePlaceChange $delta_x $delta_y
  } elseif {[string match offset $video_shape(active)]} {
    ShapePlaceOffsetChange $delta_x $delta_y
  } elseif {[string match size $video_shape(active)]} {
    ShapePlaceSizeChange $delta_x $delta_y
  } elseif {[string match freehand $video_shape(active)]} {
puts "Adding $delta_x $delta_y"
    lappend video_shape(freehand_list) "$delta_x $delta_y"
    $video_shape(canvas) create line $video_shape(x) $video_shape(y) $x $y -fill yellow -width 1 -tag freehand
  } elseif {[string match rotate $video_shape(active)]} {
    ShapePlaceRotationChange $delta_x $delta_y
  }
  set video_shape(x) $x
  set video_shape(y) $y
}
