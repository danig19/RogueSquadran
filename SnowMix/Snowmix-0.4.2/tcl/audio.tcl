proc SetMuteColor { mutebutton mute } {
  if {$mute} {
    $mutebutton configure -background red -activebackground red \
      -foreground black
  } else {
    $mutebutton configure -background darkgreen -activebackground darkgreen \
      -foreground white -activeforeground yellow
  }
}

proc MuteButton {source id sourceid} {
  global snowmix slider
  set mutebutton $slider(mixerframe,$source,$id,$sourceid).mutebutton
  set mutecolor [$mutebutton cget -background]
  set ch 0
  set ledbar $slider(mixerframe,$source,$id,$sourceid).scales.led
  set ledvar [$ledbar$ch cget -variable]
  if { [eof $snowmix] } {
    tk_messageBox -title Alert -message "Lost connection to Snowmix"
    FileMenuConnect normal
    return
  }
  if {[string match red $mutecolor]} {
    if {[string match mixersource $source]} {
      set command "audio mixer source mute off $id $sourceid"
    } else {
      set command "audio $source mute off $id"
    }
    if { [catch { puts $snowmix $command }] } {
      tk_messageBox -title Alert -message "Failed to send command to Snowmix"
      FileMenuConnect normal
      return
    }
    SetMuteColor $mutebutton 0
    gets $snowmix line
    set $ledvar 100
  } else {
    if {[string match mixersource $source]} {
      set command "audio mixer source mute on $id $sourceid"
    } else {
      set command "audio $source mute on $id"
    }
    if { [catch { puts $snowmix $command }] } {
      tk_messageBox -title Alert -message "Failed to send command to Snowmix"
      FileMenuConnect normal
      return
    }
    SetMuteColor $mutebutton 1
    gets $snowmix line
    set $ledvar 10
  }
}

proc LinkSlider { index linkbutton } {
  global audio_volume_link
  puts "Link Slider $index"
  if {[info exist audio_volume_link($index)]} {
    if {$audio_volume_link($index)} {
      set audio_volume_link($index) 0
      $linkbutton configure -text Link
    } else {
      set audio_volume_link($index) 1
      $linkbutton configure -text Unlink
    }
  } else {
    set audio_volume_link($index) 1
    $linkbutton configure -text Unlink
  }
  puts "Link $index $audio_volume_link($index)"
}

proc MakeSlider { mixerframe name source id id2 volumeindex delay mute channels scale_w scale_l} {
  global mixercount @mixer_val @mixer_mute @mixer_fade @mixer_speed
  global @mixer_lock vol_meter audio_state audio_rate
  global audioled audioledbar audio_volume
  global slider

  incr mixercount
  set mixerframe $mixerframe.mixer$mixercount
  frame $mixerframe -padx 10
  #label $mixerframe.label -text $name -background red -relief raised -borderwidth 3
  frame $mixerframe.scales

  set mixer_val($mixerframe.scale.scale1) 0
  set mixer_val($mixerframe.scale.scale2) 0
  set mixer_lock($mixerframe) 0

  set mixercolor #002000
  #frame $mixerframe.values

  if { $channels > 0 } {
    set pwidth [expr $scale_w + 5]

    ## Make progressbar for showing delay
    ProgressBar $mixerframe.scales.progress -orient vertical -width $pwidth \
      -height $scale_l -variable $delay -borderwidth 4 -fg lightblue -bg red \
      -troughcolor black -relief flat
    pack $mixerframe.scales.progress -side left

    ## Make slider, first will get number ticks
    set tick 20
    set channel 0
    while { $channel < $channels } {
      set volvar [format "audio_volume(%s,%s)" $volumeindex $channel]
      scale $mixerframe.scales.scale$channel -from 100 -to 0 -length $scale_l \
        -variable $volvar -orient vertical -tickinterval $tick -showval 0 \
        -troughcolor $mixercolor -width $scale_w \
        -command "SliderChange $mixerframe $source $id $id2 $channel $channels"
      pack $mixerframe.scales.scale$channel -side left
      #set mixer_cur($mixerframe.scale.scale$channel) $mixer_val($mixerframe.scale.scale$channel)
      incr channel
      set tick 0
    }
    set channel 0
    while { $channel < $channels } {
#puts "LEDBAR - VOLUMEINDEX=$volumeindex,$channel"
      set audioled($volumeindex,$channel) 50
      set ledvar [format "audioled(%s,%s)" $volumeindex $channel]
      set audioledbar($volumeindex,$channel) \
         [ProgressBar $mixerframe.scales.led$channel -orient vertical -width $pwidth \
           -height $scale_l -variable $ledvar -borderwidth 4 -fg green -bg red \
           -troughcolor black -relief flat ]
      pack $mixerframe.scales.led$channel -side left
      incr channel
    }

  }

  upvar $mute muted
puts "MIXERFRAME slider(mixerframe,$source,$id,$id2)"
  set slider(mixerframe,$source,$id,$id2) $mixerframe
  Button $mixerframe.mutebutton -text $name -borderwidth 2 \
    -background darkgreen -foreground black \
    -activeforeground yellow -pady -2 -activebackground darkgreen \
    -armcommand "MuteButton $source $id $id2"
  SetMuteColor $mixerframe.mutebutton $muted

  label $mixerframe.state -text state -relief sunken -padx 20
  frame $mixerframe.space1 -height 5
  frame $mixerframe.space2 -height 5
  pack $mixerframe.mutebutton $mixerframe.space1 $mixerframe.state $mixerframe.space2 -side top

  frame $mixerframe.vars
  frame $mixerframe.vars.texts
  frame $mixerframe.vars.vals

  label $mixerframe.vars.texts.sps     -text "per sec"  -background grey -anchor w
  label $mixerframe.vars.texts.silence -text "silence"  -background grey -anchor w
  label $mixerframe.vars.texts.dropped -text "dropped"  -background grey -anchor w
  label $mixerframe.vars.texts.samples -text "samples"  -background grey -anchor w
  label $mixerframe.vars.texts.delay   -text "buf ms"   -background grey -anchor w
  pack $mixerframe.vars.texts.sps $mixerframe.vars.texts.silence $mixerframe.vars.texts.dropped $mixerframe.vars.texts.samples $mixerframe.vars.texts.delay -side top -fill x -anchor w
  label $mixerframe.vars.vals.sps     -text "0" -background grey -anchor e -relief sunken -width 9
  label $mixerframe.vars.vals.silence -text "0" -background grey -anchor e -relief sunken
  label $mixerframe.vars.vals.dropped -text "0" -background grey -anchor e -relief sunken
  label $mixerframe.vars.vals.samples -text "0" -background grey -anchor e -relief sunken
  label $mixerframe.vars.vals.delay   -text "0" -background grey -anchor e -relief sunken
  pack $mixerframe.vars.vals.sps $mixerframe.vars.vals.silence $mixerframe.vars.vals.dropped $mixerframe.vars.vals.samples $mixerframe.vars.vals.delay -side top -fill x -anchor w
  pack $mixerframe.vars.texts $mixerframe.vars.vals -side left -fill x
  
  # Create a list of channels
  set ch 0
  set ch_list ""
  while { $ch < $channels } { append ch_list "$ch " ; incr ch }
  if { $channels < 2 } { set link_state disabled
  } else { set link_state active }

  frame $mixerframe.buttons -borderwidth 1
  set col1 [frame $mixerframe.buttons.col1]
  set col2 [frame $mixerframe.buttons.col2]
  Button $col1.fadeup   -text "Up"    -borderwidth 2 -padx -0 -pady -1 -width 5 \
    -armcommand "FadeSliderList $mixerframe $source $id $id2 \"$ch_list\" 1 50"
  Button $col2.fadedown -text "Down"  -borderwidth 2 -padx -0 -pady -1 -width 5 \
    -armcommand "FadeSliderList $mixerframe $source $id $id2 \"$ch_list\" 0 0"
  Button $col1.preset1  -text "Pre 1" -borderwidth 2 -padx -0 -pady -1 -state disabled
  Button $col2.link     -text "Link" -borderwidth 2 -padx -0 -pady -1 \
      -armcommand "LinkSlider $volumeindex $col2.link" -state $link_state
  Button $col1.reset    -text "Reset" -borderwidth 2 -pady -1  -width 5 \
      -armcommand "SliderListChange $mixerframe $source $id $id2 \"$ch_list\" 50 ; set_vol $volumeindex \"$ch_list\" 50"
  Button $col2.lock     -text "Lock"  -borderwidth 2 -pady -1 -width 5 \
    -armcommand "LockSlider $mixerframe.mutebutton $mixerframe.scales.scale $channels $col2.lock"
  Button $col1.add      -text "+50ms" -borderwidth 2 -pady -1  -width 5 \
    -armcommand "AddSilence $source $id $id2 50"
  Button $col2.drop     -text "-30ms" -borderwidth 2 -pady -1  -width 5 \
    -armcommand "AddSilence $source $id $id2 -30"
  pack $col1.fadeup $col1.preset1 $col1.reset $col1.add -side top -fill x -anchor n
  pack $col2.fadedown $col2.link $col2.lock $col2.drop -side top -fill x -anchor n
  pack $col1 $col2 -side left -fill x -anchor n

  set vol_meter($source,$id,$id2) $mixerframe.scales.led1.container.vol
  set audio_state($source,$id,$id2) $mixerframe.state
  set audio_rate($source,$id,$id2) $mixerframe.vars.vals

  pack $mixerframe.scales -side top
  pack $mixerframe.buttons $mixerframe.vars -side top -fill x
  return $mixerframe
}

proc AddSilence {source id1 id2 ms} {
  global snowmix
  if { $ms >= 0 } { set command "add silence"
  } else { set command "drop"
    set ms [expr -$ms]
  }
  if {[string match mixersource $source]} {
    puts $snowmix "audio mixer source $command $id1 $id2 $ms"
  } else {
    puts $snowmix "audio $source $command $id1 $ms"
  }
  gets $snowmix line
}

proc FadeSliderList { mixer source id id2 ch_list direction target } {
  set channels [llength $ch_list]
  foreach channel $ch_list {
    FadeSlider $mixer $source $id $id2 $channel $channels $direction $target
  }
}
  
proc FadeSlider {mixer source id id2 channel channels dir target} {
  global audio_volume
  if {[string match feed $source] || [string match mixer $source] || [string match sink $source]} {
    set index "$source,$id,$channel"
  } else {
    set index "mixer,$id,$id2,$channel"
  }
  set vol $audio_volume($index)
  if {$dir == 0 && $vol > $target} {
    incr vol -2
    if {$vol < $target} {
      set audio_volume($index) $target
    } else {
      set audio_volume($index) $vol
      SliderChange $mixer $source $id $id2 $channel $channels $vol
      after 50 FadeSlider $mixer $source $id $id2 $channel $channels $dir $target
    }
  } elseif {$dir == 1 && $vol < $target} {
    incr vol 2
    if {$vol > $target} {
      set audio_volume($index) $target
    } else {
      set audio_volume($index) $vol
      SliderChange $mixer $source $id $id2 $channel $channels $vol
      after 50 FadeSlider $mixer $source $id $id2 $channel $channels $dir $target
    }
  }
  #puts "VOL $vol"
}

proc LockSlider {mute slider channels lock} {
  #global $slider $lock
  set ch 0
  set sliderstate [$slider$ch cget -state]
  if {[string match Lock [$lock cget -text]]} {
    $lock configure -text Unlock -bg #d90909 -activebackground #d90909
    while { $ch < $channels } {
      $slider$ch configure -state disabled -troughcolor darkgrey -fg grey 
      incr ch
    }
    $mute configure -state disabled
  } else { 
    $lock configure -text Lock -bg #d9d9d9 -activebackground darkgreen
    while { $ch < $channels } {
      $slider$ch configure -state normal -troughcolor black -fg #5f5f5f -activebackground darkgreen
      incr ch
    }
    $mute configure -state normal
  }
}

# Set volume
proc set_vol {volume_index ch_list vol} {
  global audio_volume
puts "SETTING VOLUME : $volume_index - $ch_list - $vol"
  foreach channel $ch_list {
    set audio_volume($volume_index,$channel) $vol
  }
}

proc audio_pane { pane scale_w scale_l} {
  global audiopane audiofeeds
  # Get settings for mixer
  GetAudioSettings { feed mixer sink }
  # Create a Notebook panel for feeds, mixers and sinks
  set audiopane [NoteBook $pane.nb -side top -arcradius 4 -homogeneous 1 -tabbevelsize 2]

  set pane_no 1
  set no 1
  set max_no 7
  set feed_count [llength $audiofeeds ]
  while { $no <= $feed_count } {
    if { [expr $no + $max_no] > $feed_count } { set end_no $feed_count } else { set end_no [expr $no + $max_no - 1] }
    $audiopane insert end feeds$pane_no -text " Audio Feeds $no - $end_no"
    audio_feed feed [$audiopane getframe feeds$pane_no] $scale_w $scale_l $no $end_no
    incr pane_no
    incr no $max_no
  }
  audio_mixer $audiopane $scale_w $scale_l
  $audiopane insert end sinks -text " Audio Sinks"
  audio_feed sink [$audiopane getframe sinks] $scale_w $scale_l -1 10000
  pack $audiopane -side top -fill x
  catch { $audiopane raise feeds1 }
  #catch { $audiopane raise mixer0 }
}

proc audio_mixer { audiopane scale_w scale_l} {
  global snowmix
  global audio_volume audio_mute audio_name audio_channels
  global audiomixers audiomixer_source
  global audio_delay

#puts "audiomixers : $audiomixers"

#puts "sources: [array names audiomixer_source]"
#puts "sources: [array get audiomixer_source]"

  #set mixerpane [NoteBook $panel.nb -side top -arcradius 4]
  # $nb insert 0 overview   -text "Overview"
  # set pane [$nb getframe textfont]

  set i 0
  foreach id $audiomixers {
    $audiopane insert end mixer$i -text " $audio_name(mixer,$id)"
    set pane [$audiopane getframe mixer$i]
    puts "AUDIO MIXER $id"

    ScrollableFrame $pane.scrollframe -constrainedwidth false -constrainedheight false \
      -xscrollcommand "$pane.scrollbar set"
    #  -xscrollcommand "$pane.scrollbar set" -width 1200 -height 408
    scrollbar $pane.scrollbar -command "$pane.scrollframe xview" -orient horizontal
    pack $pane.scrollframe -side top -fill both -expand 1
    pack $pane.scrollbar -side top -fill x

    #set panefr [frame $pane.scrollframe.mixer$id -relief raised -borderwidth 2]
    set panefr [$pane.scrollframe getframe]
    #pack $panefr -side top -padx 4 -anchor ne

    # Buttons on the left
    frame $panefr.leftcontrol
    frame $panefr.leftcontrol.space -width 10 -height 40 
    set buttons [frame $panefr.leftcontrol.buttons -borderwidth 0 -relief sunken]
    label $buttons.label -text "Mixer $id" -font {-weight bold} -relief raised -borderwidth 2 -padx 10 -pady 1
    Separator $buttons.hsep -orient horizontal
    Button $buttons.fadeup   -text "Fade Up"   -borderwidth 2 -pady 0 -state disabled 
    Button $buttons.fadedown -text "Fade Down" -borderwidth 2 -pady -4 -state disabled
    Button $buttons.lock     -text "Lock"      -borderwidth 2 -pady 0 -state disabled
    Button $buttons.preset1  -text "Preset 1"  -borderwidth 2 -pady 0 -state disabled
    Button $buttons.preset2  -text "Preset 2"  -borderwidth 2 -pady 0 -state disabled
    Button $buttons.learn    -text "Learn"     -borderwidth 2 -pady 0 -state disabled
    Button $buttons.speed    -text "Speed"     -borderwidth 2 -pady 0 -state disabled
    pack $buttons.label -side top -fill x -pady 2
    pack $buttons.hsep -side top -fill x -pady 10
    pack $buttons.fadeup $buttons.fadedown $buttons.lock $buttons.preset1 \
      $buttons.preset2 $buttons.learn $buttons.speed -side top -fill x -pady 1
    #pack $panefr.leftcontrol.space $buttons -side top -fill x -anchor n
    pack $buttons -side top -fill x -anchor n
    pack $panefr.leftcontrol -side left -fill x -anchor ne
    set slides [frame $panefr.slides]
    set source_no 1
#puts "FOREACH $audiomixer_source($id)"
    foreach slide $audiomixer_source($id) {
#puts " - slide=$slide"
      #regsub -all "," $slide " " slide_list
      set source [lindex $slide 0]
      set source_id [lindex $slide 1]
set source_count [lindex $slide 2]
#puts " - SETTING SLIDENAME : id=$id slide=$slide source=$source source_id=$source_id source_no=$source_no"
      if {[string match feed $source]} {
      	#set slidename $audio_name(feed,$source_id)
      	#set channels $audio_channels(feed,$source_id)
set slidename $audio_name(feed,$source_id)
set channels $audio_channels(feed,$source_id)
      } else {
      	set slidename $audio_name(mixer,$source_id)
      	set channels $audio_channels(mixer,$source_id)
#      	set slidename $audio_name(mixer,$source_id)
#      	set channels $audio_channels(mixer,$source_id)
      }
#puts "done"
      set slideindex "mixer,$id,$slide"
      set slideindex "mixer,$id,$source_id"
set slideindex "mixer,$id,$source_count"


#puts "SLIDE=$slide"
#puts "SLIDEINDEX=$slideindex"
#puts "\nslides=$slides"
#puts "slidename=$slidename"
#puts "id=$id"
#puts "source_no=$source_no"
#puts "slideindex=$slideindex"
#puts "delay=audio_delay(mixer,$id,$source_no)"
#puts "mute=audio_mute($slideindex)"
#puts "channels=$channels"
#puts "scale_w=$scale_w"
#puts "scale_l=$scale_l"
#puts "mute array = [array get audio_mute]"
      set slide [MakeSlider $slides $slidename mixersource $id $source_no $slideindex \
           audio_delay(mixer,$id,$source_no) audio_mute($slideindex) \
	$channels $scale_w $scale_l]
      set audio_delay(mixer,$id,$source_no) 0
      pack $slide -side left -expand false -fill y
      incr source_no

    }
    set slide [MakeSlider $slides $audio_name(mixer,$id) mixer $id 0 \
	"mixer,$id" audio_delay(mixer,$id,0) audio_mute(mixer,$id) \
	$audio_channels(mixer,$id) $scale_w $scale_l]
    set audio_delay(mixer,$id,0) 0
    #set space [frame $panefr.space -pady 10]
    set vsep [Separator $panefr.slides.vsep -orient vertical]
    pack $vsep -side left -fill y
    pack $slide -side left -expand false -fill y
    #pack $panefr.label -side top -pady 5
    pack $slides -side top
    incr i
  }
}

proc GetAudioSettings { audiotypes } {
  global snowmix
  global audio_volume audio_volume_previous audio_mute audio_name audio_channels
  global audiofeeds audiosinks audiomixers audiomixer_source

  #set audiotypes feed
  #lappend audiotypes sink
  #lappend audiotypes mixer
  foreach type $audiotypes {

    # Get audio feed|sink|mixer names
    puts $snowmix "audio $type add"
    while {[gets $snowmix line] >= 0} {
      if {[string compare "STAT: " $line] == 0} {
	  break
      }
      if {[regexp {[^0-9]+([0-9]+)\ <([^>]*)} $line match id name]} {
        if {[string match feed $type]} {
          lappend audiofeeds "$id"
          set audio_name($type,$id) $name
        } elseif {[string match sink $type]} {
          lappend audiosinks "$id"
          set audio_name($type,$id) $name
        } elseif {[string match mixer $type]} {
          lappend audiomixers "$id"
          set audio_name($type,$id) $name
          set source_count 0
        }
      }
    }

    # Get audio feed|sink|mixer channels
    puts $snowmix "audio $type channels"
    while {[gets $snowmix line] >= 0} {
      if {[string compare "STAT: " $line] == 0} {
	  break
      }
      if {[regexp {[^0-9]+([0-9]+)\ channels +([0-9]+)} $line match id channels]} {
        set audio_channels($type,$id) $channels
      }
    }

    if {[string match mixer $type]} {
      # Get Audio mixer sources
      puts $snowmix "audio mixer source"
      set last_id -1
      set source_count 0
      while {[gets $snowmix line] >= 0} {
        if {[string compare "STAT: " $line] == 0} {
	    break
        }
        if {[regexp {audio mixer\ +([0-9]+)\ +sourced by audio\ +([^ ]+)[^0-9]+([0-9]+)} $line match id source source_id]} {
          if {$id != $last_id} {
            set source_count 1
            set last_id $id
            set audiomixer_source($id) ""
          } else {
            incr source_count
          }
          lappend audiomixer_source($id) "$source $source_id $source_count"
        }
      }
    }
    GetAudioVolume $type
  }
}

proc GetAudioVolume { type } {
  global snowmix slider audio_volume audio_volume_previous audio_mute
  global audiomixer_source
  #audiomixer_source_volume

  set mixer 0
  # Get audio feed|sink|mixer volume
  if { [catch { puts $snowmix "audio $type volume" }] } {
    return
  }

  while {[gets $snowmix line] >= 0} {
    if {[string compare "STAT: " $line] == 0} {
      break
    }
    set muted ""
    # Volume for feed and sink
    if {![string match mixer $type]} {
      regexp {[^0-9]+([0-9]+)\ volume +([\.,0-9]+)( muted)*} $line match id volume muted
      regsub -all "," $volume " " volume_list
      set channel 0
      foreach volume $volume_list {
        if {$volume < 0} { set volume 0}
        if {$volume > 4} { set volume 4}
        set audio_volume($type,$id,$channel) [expr round(sqrt(2500*$volume))]
        set audio_volume_previous($type,$id,$channel) $audio_volume($type,$id,$channel)
        if {[string compare " muted" $muted] == 0} {
          set audio_mute($type,$id) 1
        } else {
          set audio_mute($type,$id) 0
        }
        if {[info exist slider(mixerframe,$type,$id,0)]} {
          SetMuteColor $slider(mixerframe,$type,$id,0).mutebutton \
            $audio_mute($type,$id)
        }
        incr channel
      }

    # Volume for mixer
    } else {
      set source ""
      set volume 0
      set match ""
      set id ""
      set muted ""
      # audio mixer  2 volume 127 muted
      # - source audio feed   1 volume 127

      # Check if this is a source line for the mixer
      if {[string match {STAT: -*} $line]} {
        regexp {audio ([^ ]+) +([0-9]+)\ volume +([\.,0-9]+)(\ muted)*} $line match source id volume muted
        regsub -all "," $volume " " volume_list
        set channel 0
        incr source_count
        foreach volume $volume_list {
          if {$volume < 0} { set volume 0}
          if {$volume > 4} { set volume 4}
          set audio_volume(mixer,$mixer,$source_count,$channel) \
            [expr round(sqrt(2500*$volume))]
          set audio_volume_previous(mixer,$mixer,$source_count,$channel) \
            $audio_volume(mixer,$mixer,$source_count,$channel)
          incr channel
        }
        if {[string compare " muted" $muted] == 0} {
          set audio_mute(mixer,$mixer,$source_count) 1
        } else {
          set audio_mute(mixer,$mixer,$source_count) 0
        }
        if {[info exist slider(mixerframe,mixersource,$mixer,$source_count)]} {
           SetMuteColor $slider(mixerframe,mixersource,$mixer,$source_count).mutebutton \
             $audio_mute(mixer,$mixer,$source_count)
        }
        continue

      # else this is the main line for the mixer
      } else {
        regexp {[^0-9]+([0-9]+)\ volume +([\.,0-9]+)( muted)*} $line match mixer volume muted
        regsub -all "," $volume " " volume_list
        set channel 0
        set source_count 0

        foreach volume $volume_list {
          if {$volume < 0} { set volume 0}
          if {$volume > 4} { set volume 4}
          set audio_volume(mixer,$mixer,$channel) \
            [expr round(sqrt(2500*$volume))]
          set audio_volume_previous(mixer,$mixer,$channel) \
            $audio_volume(mixer,$mixer,$channel)
          incr channel
        }
        if {[string compare " muted" $muted] == 0} {
          set audio_mute(mixer,$mixer) 1
        } else {
          set audio_mute(mixer,$mixer) 0
        }
        if {[info exist slider(mixerframe,$type,$mixer,0)]} {
          SetMuteColor $slider(mixerframe,$type,$mixer,0).mutebutton \
            $audio_mute(mixer,$mixer)
        }
      }
    }
  }
}

proc audio_feed { type pane scale_w scale_l start stop} {
  global snowmix
  global audio_volume audio_mute audio_name audio_channels
  global audiofeeds audio_delay
  global audiosinks

  #puts "feeds  : $audiofeeds"
  #puts "volume : [array names audio_volume]"
  #puts "volume : [array get audiofeed_volume]"

  ScrollableFrame $pane.scrollframe -constrainedwidth false -constrainedheight false \
    -xscrollcommand "$pane.scrollbar set" -width 1200 -height 408
  scrollbar $pane.scrollbar -command "$pane.scrollframe xview" -orient horizontal
  pack $pane.scrollframe -side top
  pack $pane.scrollbar -side top -fill x

  set panefr [$pane.scrollframe getframe]

  if {[string match feed $type]} {
    set n 0
    foreach id $audiofeeds {
      incr n
      if {$n < $start || $n > $stop} continue
      #set slide slide$id
      set slide [MakeSlider $panefr "$id - $audio_name($type,$id)" $type $id 0 "$type,$id" \
	   audio_delay($type,$id,0) audio_mute($type,$id) $audio_channels($type,$id) $scale_w $scale_l]
      set audio_delay(feed,$id,0) 0
      pack $slide -side left -expand false -fill y
    }


  } elseif {[string match sink $type]} {
    foreach id $audiosinks {
      #set slide slide$id
      set slide [MakeSlider $panefr $audio_name($type,$id) $type $id 0 "$type,$id"  \
	   audio_delay(sink,$id,0) audio_mute($type,$id) $audio_channels($type,$id) $scale_w $scale_l]
      set audio_delay(sink,$id,0) 0
      pack $slide -side left -expand false -fill y
    }
  }
}

proc ScaleButton {} {
  puts "scale button\n"
}

proc ScaleDown { mixer } {
  global out2
puts "scale down $mixer $out2\n"
  if {$out2 > 0} {
puts "scale down \n"
    set out2 [expr $out2 - 1]
    after 100 ScaleDown $mixer
  }
}


proc MixerLock2 {mixer} {
  global out1 out2 [$mixer.lock cget -variable]
  upvar [$mixer.lock cget -variable] value
  puts "Mixer lock $value\n"
  if {$value} {
    $mixer.scales.scale2 config -variable out1
  } else {
    set out2 $out1
    $mixer.scales.scale2 config -variable out2
    set command "ScaleDown $mixer"
puts "scale down soon $command\n"
    after 70 $command 
puts "scale down soooon \n"
  }
}

proc MixerMute {mixer} {
  global [$mixer.mute cget -variable]
  upvar [$mixer.mute cget -variable] value
  puts "Mixer mute $value\n"
  if {$value} { $mixer.label config -background red } else { $mixer.label config -background green }
}

proc MixerLock {mixer} {
  global @mixer_lock
#  puts "MixerLock mixer $mixer [array names mixer_lock] \n"
  if {$mixer_lock($mixer) == 0}
  if {$mixer_lock($mixer) != 0} {
    puts "Unlocking mixer $mixer\n"
#    set mixer_lock($mixer) false
  } else {
    puts "Locking mixer $mixer\n"
#    set mixer_lock($mixer) true
  }
}

proc SliderListChange {mixer source id id2 ch_list value } {
  set channels [llength $ch_list]
  foreach ch $ch_list {
    SliderChange $mixer $source $id $id2 $ch $channels $value
  }
}

proc SliderChange {mixer source id id2 channel channels value } {
  global snowmix2 audio_volume audio_volume_previous audio_channels audio_volume_link
  set volume [expr $value*$value/2500.0]
#  puts "PMM MIXER mixer=$mixer\nsource=$source\nid=$id\nid2=$id2\nchannel=$channel\nvalue=value"
#puts "SliderChange source=$source id=$id id2=$id2 value=$value"
  if {[string match mixersource $source]} { set volumeindex mixer,$id,$id2
  } else { set volumeindex $source,$id }

  if {[info exist audio_volume($volumeindex,$channel)]} {
    #puts "Volume change from $audio_volume_previous($volumeindex,$channel) $audio_volume($volumeindex,$channel)"
  } else {
    puts "no audio_volume($volumeindex,$channel) $mixer $source $id $id2 $channel $channels $value"
  }
  #puts "CHANGE index $volumeindex channels=$channels channel=$channel"
  if {[info exist audio_volume_link($volumeindex)]} {
    if {$audio_volume_link($volumeindex)} {
      set ch 0
      set delta [expr $audio_volume($volumeindex,$channel) - $audio_volume_previous($volumeindex,$channel)]
      set audio_volume_link($volumeindex) 0
      while { $ch < $channels } {
        if { $ch != $channel } {
          set newvolume [expr $audio_volume($volumeindex,$ch) + $delta]
          set audio_volume($volumeindex,$ch) $newvolume
          SliderChange $mixer $source $id $id2 $ch $channels $newvolume
        }
        incr ch
      }
      set audio_volume_link($volumeindex) 1
    }
  }
 
  if {[info exist audio_volume($volumeindex,$channel)]} {
    set audio_volume_previous($volumeindex,$channel) $audio_volume($volumeindex,$channel)
  } else {
    puts "Can not get audio_volume($volumeindex,$channel)\n"
  }
  set ch 0
  set volume_list ""
  while { $ch < $channel } { append volume_list "- " ; incr ch}
  append volume_list $volume
  if {[string match feed $source] || [string match mixer $source] || [string match sink $source]} {
    #puts "audio $source volume $id $volume_list"
    puts $snowmix2 "audio $source volume $id $volume_list"
    gets $snowmix2 line
  } elseif {[string match mixersource $source]} {
    puts $snowmix2 "audio mixer source volume $id $id2 $volume_list"
    gets $snowmix2 line
  } else {
    puts "unknown source for mixer change"
  }
  #puts "=====================================\n"
}

proc MixerChange {mixer slide value } {
  global @mixer_val  @mixer_lock @mixer_mute @mixer_fade @mixer_speed
  set volume [expr round($value*$value*255/10000)]
  puts "Mixer Change for mixer $mixer slide $slide to $value volume $volume"
  #puts "Mixer [$mixer config -variable] change to $value\n"
  #if {$value eq 80} { $mixer config -variable out1 ; set mute 0}
}

proc get_audio_feeds {} {
	global snowmix
	puts "write to socket\n"
	puts $snowmix "audio feed add"
	puts "written to socket\n"
}

proc StatusSample { } {
  global nb audiopane
  if {[string match "audio" [$nb raise]]} {
    if {[string match "feeds*" [$audiopane raise]]} {
      ProcessAudio feed feed
    } elseif {[string match "sinks" [$audiopane raise]]} {
      ProcessAudio sink sink
    } else {
      ProcessAudio mixer mixersource
    }
  } elseif {[string match "audiofeed" [$nb raise]]} {
    ProcessAudio feed feed
  } elseif {[string match "audiomixer" [$nb raise]]} {
    ProcessAudio mixer mixersource
  } elseif {[string match "audiosink" [$nb raise]]} {
    ProcessAudio sink sink
  }
  after 100 StatusSample
}

proc ProcessAudio { source1 source2 } {
  global vol_meter audio_delay audio_state audio_rate snowmix
  global audioled audioledbar audio_volume audio_volume_previous
  # First we ask for source status
  if { [eof $snowmix] } {
    FileMenuConnect normal
  }

  if { [catch { puts $snowmix "audio $source1 status" }] } { return }
  
  set id -1
  set source_count 0
  while {[gets $snowmix line] >= 0} {
    if {[string compare "STAT: " $line] == 0} {
	break
    }
    set state ""
    set samples 0
    set silence 0
    set dropped 0
    set clipped 0
    set rms ""
    #                id          state    samples   silence   dropped   clipped
    # feed_id : state samples samplespersecond avg_samplespersecond silence dropped clipped rms
    if {[string match "STAT: audio*" $line]} {
      set source 0
      set id -1
      set delay -1
      if {[regexp {[^0-9]+([0-9]+)\ :\ ([^ ]+)\ +([0-9]+)\ +([0-9]+)\ +(\-*[0-9]+)\ +([0-9]+)\ +([0-9]+)\ +([0-9]+)\ +([,\-\.0-9]+)\ +([\.,0-9]+)} $line match id state samples samplespersecond avg_samplespersecond silence dropped clipped delay rms] } {
        set source_count 0
      }
    } else {
      set source -1
      if {[regexp {[^0-9]+([0-9]+)\ :\ ([^ ]+)\ +([0-9]+)\ +([0-9]+)\ +(\-*[0-9]+)\ +([0-9]+)\ +([0-9]+)\ +([0-9]+)\ +([\-\.0-9]+)\ +([\.,0-9]+)} $line match source state samples samplespersecond avg_samplespersecond silence dropped clipped delay rms]} {
        incr source_count
      }
      
    }
#puts "source1=$source1 id=$id source=$source <$line>"
    if { $id < 0 || $source < 0} continue
    set maxdelay $delay
#puts "PROCESS : Source=$source id=$id source2=$source2"
    if {[string match "*,*" $delay]} {
      set maxdelay 0
      set rest $delay
      while {[string length $rest] > 0} {
        if {[regexp {([0-9]+),*([0-9,]*)} $rest match delay1 rest1]} {
          if {$delay1 > $maxdelay} {set maxdelay $delay1}
        }
	set rest $rest1
      }
    }
    set bg_state_color red
    set fg_state_color black
    if {[string match {RU*} $state]} { set bg_state_color darkgreen
      set fg_state_color white
    } elseif {[string match {PE*} $state]} { set bg_state_color green
    } elseif {[string match {RE*} $state]} { set bg_state_color lightblue
    } elseif {[string match {SE*} $state]} { set bg_state_color yellow
    } elseif {[string match {ST*} $state]} { set bg_state_color red
    }
    if { $source == 0} {
      set vol $vol_meter($source1,$id,$source)
      set astate $audio_state($source1,$id,$source)
      set rate $audio_rate($source1,$id,$source)
    } else {
#     set vol $vol_meter($source2,$id,$source)
#      set astate $audio_state($source2,$id,$source)
#      set rate $audio_rate($source2,$id,$source)
      set vol $vol_meter($source2,$id,$source_count)
      set astate $audio_state($source2,$id,$source_count)
      set rate $audio_rate($source2,$id,$source_count)
    }
    if {$maxdelay >= 0} {
      set delaysquared [expr 2*round(sqrt($maxdelay))]
      if { $delaysquared > 100 } { set $delaysquared 100 }
      #set audio_delay($source1,$id,$source) $delaysquared
set audio_delay($source1,$id,$source_count) $delaysquared
    }
    #if {[string match sink $source1]} {puts "$astate configure -text $state -background $state_color"}
    $astate configure -text $state -background $bg_state_color -foreground $fg_state_color
    $rate.sps configure -text "$avg_samplespersecond"
    $rate.silence configure -text "$silence"
    $rate.dropped configure -text "$dropped"
    if {$avg_samplespersecond > 0} {
      set samples [format "%.1f s" [expr 1.0 * $samples / $avg_samplespersecond]]
    }
    $rate.samples configure -text "$samples"
    $rate.delay configure -text "$delay ms"

    regsub -all "," $rms " " rms_list
    set channel 0
    foreach irms $rms_list {
      if { $irms < 2 } { set irms 2 }
      if { $irms > 100 } { set irms 100 }
      if { $irms < 70 } { set fg_color green
      } elseif { $irms < 80 } { set fg_color yellow
      } elseif { $irms < 90 } { set fg_color magenta
      } else { set fg_color red }
      if { $clipped > 0 } { set fg_color red }

      if {$source_count > 0 } {
#        puts "audioled($source1,$id,$source,$channel) $irms"
        set audioled($source1,$id,$source_count,$channel) $irms
#puts "set audioled($source1,$id,$source_count,$channel) $irms"
        if {[info exist audioledbar($source1,$id,$source_count,$channel)]} {
          $audioledbar($source1,$id,$source_count,$channel) configure -fg $fg_color
        } else {
puts "audioledbar($source1,$id,$source_count,$channel) does not exist"
puts "ARRAY [array get audioledbar]"
        }
      } else {
        set audioled($source1,$id,$channel) $irms
        if {[info exist audioledbar($source1,$id,$channel)]} {
          $audioledbar($source1,$id,$channel) configure -fg $fg_color
	}
      }
      incr channel
    }
  }
  GetAudioVolume $source1
}
