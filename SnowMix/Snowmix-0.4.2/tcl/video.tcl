proc GetFeeds {} {
  global video snowmix
  set video(feed,max_id) -1
  puts $snowmix "feed info"
  while {[gets $snowmix line] >= 0} {
    if {[string compare "STAT: " $line] == 0} break
    # STAT: feed id : state islive oneshot geometry cutstart cutsize offset fifo good missed dropped <name>
    # STAT: feed 0 : STALLED recorded continuously 1280x720 0,0 1280x720 0,0 0:0 0 20448 0 <Internal>
    if {[regexp {STAT:\ +feed\ +([0-9+])\ +:\ +([^ \t]+)\ +([^ \t]+)\ +([^ \t]+)\ +([0-9]+)x([0-9]+)\ +([0-9]+),([0-9]+)\ +([0-9]+)x([0-9]+)\ +([0-9]+),([0-9]+)\ +([0-9:]+)\ +([0-9]+)\ +([0-9]+)\ +([0-9]+)\ +<([^>]+)} $line match feed_id state live contnously width height cut_col cut_row cut_width cut_height off_col off_row fifo good missed dropped name] } {
puts "GOT FEED $feed_id"
      lappend video(feeds) $feed_id
    }
  }
  if {$feed_id > -1} {
    set video(feed,max_id) $feed_id
  }
  return
}

proc video_feeds { pane } {
  global video
  if { [string length $video(feeds)] == 0} {
    label $pane.label -text "No video feeds"
    pack $pane.label -side top -fill x
    return
  }
  foreach feed_id $video(feeds) {
    set fr [frame $pane.feed$feed_id]
    label $fr.label -text "Feed $feed_id"
    pack $fr.label -side top -fill x
    pack $fr -side left
  }
}

proc video_pane { pane } {
  global snowmix video
  set video(feeds) ""
  set video(vfeeds) ""
  GetFeeds

  ScrollableFrame $pane.scrollframe -constrainedwidth false -constrainedheight false \
    -xscrollcommand "$pane.scrollbar set" -width 1200 -height 408 -bg red
  scrollbar $pane.scrollbar -command "$pane.scrollframe xview" -orient horizontal
  pack $pane.scrollframe -side top -fill x -expand 1
  pack $pane.scrollbar -side top -fill x
  set panefr [$pane.scrollframe getframe]

  set sp [NoteBook $panefr.nb -side top -arcradius 4 -homogeneous 1 -tabbevelsize 2]
  pack $sp -side top -fill x -anchor w -expand 1

  $sp insert end feeds  -text "Video Feeds"
  $sp insert end vfeeds -text "Virtual Feeds"

  set feedpane [$sp getframe feeds]
  video_feeds $feedpane

  set feedpane [$sp getframe vfeeds]
  label $feedpane.label -text "hello world. this is a text"
  pack $feedpane.label -side top -expand 1
  
#  foreach scene_id $scene(scenes) {
#    GetSceneInfo $scene_id
#    if {[info exist scene(name,$scene_id)]} {
#      set scene(tab_id,$scene_id) scene$scene_id
#      $sp insert end scene$scene_id
#      # $sp insert end scene$scene_id -text $name
#      SceneTabActive $scene_id $scene(active,$scene_id)
#      CreateScenePage $scene_id
#      #set scene_pane [$sp getframe $scene(tab_id,$scene_id)]
#      if {$scene(active,$scene_id) > 0} { $sp raise scene$scene_id }
#    }
#  }
}

