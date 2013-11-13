#!/bin/sh
# \
exec /usr/bin/wish "$0" ${1+"$@"}
#
# !/usr/bin/wish

package require BWidget
#package require Thread

# Main Variables
set snowcub_version "0.2"
set snowcub_copyright "Copyright Peter Maersk-Moller 2012.\nAll rights reserved."
set snowcub_title "Snowcub version $snowcub_version"
set snowmix_ip 193.88.237.22
set snowmix_ip 127.0.0.1
set snowmix_port 9999
if { $argc > 0 } { set snowmix_ip [lindex $argv 0] } else {
  puts "using $snowmix_ip as default ip address"
}
if { $argc > 1 } { set snowmix_port [lindex $argv 1] } else {
  puts "using $snowmix_port as default port number"
}

source [file join [file dirname [info script]] video.tcl]
source [file join [file dirname [info script]] scenes.tcl]
source [file join [file dirname [info script]] texts.tcl]
source [file join [file dirname [info script]] audio.tcl]
source [file join [file dirname [info script]] special.tcl]
source [file join [file dirname [info script]] shapes.tcl]

# Mainmenu
#set menubarentries "File Sources Mixers Filters Destinations"
set menubarentries "File Audio Text Images Video"
set menubarbg "gray"

# main Window
proc MainWindow {title} {
  . config -bg black
  wm geometry . 1000x550
  wm minsize . 400 20
  wm title . $title
}

MainWindow "$snowcub_title"

proc new_project {} {
  puts "new projects"
}
proc save_project {} {
  puts "save projects"
}
proc saveas_project {} {
  puts "saveas projects"
}
proc close_project {} {
  puts "close project"
}

proc Connect2Snowmix {ip port} {
  global snowmix
  puts Disconnecting
  catch { close $snowmix }
  if {[catch { set snowmix [OpenSocket $ip $port] } res]} {
    return
  }
  puts "Connecting $res"
  puts $snowmix "audio feed verbose 1\naudio mixer verbose 1\naudio sink verbose 1\n"
  gets $snowmix line
  gets $snowmix line
  gets $snowmix line
  FileMenuConnect disabled
}

# Create the main menu
option add *tearOff 0
menu .menubar -bg $menubarbg
. config -menu .menubar -width 300

# Apple systems
if {[string match aqua [tk windowingsystem] ]} {
  .menubar add cascade -label Snowcub -menu [menu .menubar.apple]
  .menubar.apple add command -label Quit -command exit
}

# Common menu entries
foreach m "$menubarentries" {
  set $m [menu .menubar.m$m]
#  puts "$m $File"
  .menubar add cascade -label $m -menu .menubar.m$m
}
$File add command -label New -command new_project
$File add command -label Save -command save_project
$File add command -label "Save as" -command saveas_projects
$File add command -label "Close" -command close_project
$File add separator
$File add command -label "Connect to Snowmix" -command  "Connect2Snowmix $snowmix_ip $snowmix_port" -state normal
$File add separator
bind . <Key-c> "Connect2Snowmix $snowmix_ip $snowmix_port"

proc FileMenuConnect { state } {
  global File
  set cur_state [$File entrycget 5 -state]
  if {![string match $cur_state $state]} {
    $File entryconfigure 5 -state $state
  }
  set cur_state [.menubar entrycget 6 -state]
  if {![string match $cur_state $state]} {
    .menubar entryconfigure 6 -state $state
  }
}

$Audio add command -label "Get audio feeds" -command "audio_feed feed"
#$Mixers add command -label "Close" -command close_project

# On X11 we add Quit here
if {[string match x11 [tk windowingsystem] ]} {
  $File add command -label "Quit" -command exit
}

# On Apple we add help menu
if {[string match aqua [tk windowingsystem] ]} {
  .menubar add cascade -label Help -menu [menu .menubar.help]
}

.menubar add separator
.menubar add command -label "Connect Snowmix" -command "Connect2Snowmix $snowmix_ip $snowmix_port" -state disabled
#grab .
#button .exit -text exit -command { puts stdout "exit" ; exit }
#pack .exit -padx 20 -pady 10
puts [tk appname]
puts [tk windowingsystem]

set mixercount 0
set mixer_val(0) 0
set mixer_cur(0) 0
set mixer_mute(0) 0
set mixer_lock(0) 0
set mixer_fade(0) 0
set mixer_speed(0) 0

set audiofeeds ""
set audiomixers ""
set audiosinks ""

proc entry_change { id } {
  global snowmix text_strings text_strings_copy
  puts "entry change $id $text_strings($id) to $text_strings_copy($id)"
  if {![string match $text_strings($id) $text_strings_copy($id)]} {
    set text_strings($id) $text_strings_copy($id)
    puts $snowmix "text string $id $text_strings_copy($id)"
    puts "text string $id $text_strings_copy($id)"
  }
}

proc text_placed_pane { pane } {
}

proc text_string_pane { pane } {
  global text_strings text_strings_copy
#  puts "strings  : [array get text_strings]"
  set max -1
  foreach id [array names text_strings] {
    if { $id > $max } { set max $id }
  } 
  set i 0
#if { $max > 4 } { set max 4 }
  if { $max > 4 } {
    set max 4
  }
  while {$i <= $max } {
    if { [info exists text_strings($i)]} {
      set text_strings_copy($i) $text_strings($i) 
      set container [frame $pane.textstring$i]
      label $container.id -text $i
      entry $container.entry -relief sunken -width 100 -textvariable text_strings_copy($i)
      bind $container.entry <Return> "entry_change $i"
      bind $container.entry <FocusOut> "entry_change $i"
      pack $container.id $container.entry -side left
      pack $container -side top
    }
    incr i
  }
}

proc Overview {} {
  global snowmix
  puts $snowmix "stat"
  while {[gets $snowmix line] >= 0} {
    if {[string compare "STAT: --" $line] == 0} {
	break
    }
    set match ""
    set id ""
    regexp {[^0-9]+([0-9]+)\ +<(.*)\ *>\ +([^ ]*)\ +([^ ]*)\ +([^ ]*)\ +([^ ]*)\ +([^ ]*)\ +([^ ]*)} \
      $line match id name state mode cutout size offset scale
    if {[string length $id] < 1} continue
    puts "match = <$match>"
    puts "id     = <$id>"
    puts "name   = <$name>"
    puts "state  = <$state>"
    puts "mode   = <$mode>"
    puts "cutout = <$cutout>"
    puts "size   = <$size>"
    puts "offset = <$offset>"
    puts "scale  = <$scale>"
  }
}

proc define_tabs {} {
  global nb mute snowcub_version tcl_platform @mixer_lock snowmix
  #global audio_pane text_pane

  set nb [NoteBook .nb -side top -arcradius 4 -homogeneous 1 -tabbevelsize 2]
  $nb insert 0 overview  -text " Overview"
  $nb insert end scenes  -text " Scenes"
  $nb insert end audio   -text " Audio"
  $nb insert end video   -text " Video"
  $nb insert end videoclip   -text " Shapes"
  $nb insert end text    -text " Texts"
  $nb insert end images  -text " Images"
  $nb insert end macros  -text " Macros"
  $nb insert end special -text " Suborbital"
  $nb insert end config  -text " Config"

  set scale_w 12
  set scale_l 160

  # Check for scenes
  set pane [$nb getframe scenes]
  puts $snowmix "tcl eval ScenesList"
  gets $snowmix line
  if {[string match "MSG: Invalid*" $line]} {
    label $pane.label -text "No scenes detected"
    pack $pane.label -side top
    set raise_pane audio
  } else {
    #puts "Setting scenes"
    scene_pane $pane
    set raise_pane scenes
  }

  puts "Setting audio pane"
  set pane [$nb getframe audio]
  audio_pane $pane $scale_w $scale_l

  puts "Setting video pane"
  set pane [$nb getframe video]
  #video_pane $pane 

  puts "Setting text pane"
  set pane [$nb getframe text]
  text_pane $pane

  #puts "Setting video pane"
  #set pane [$nb getframe video]
  #label $pane.label -text "Under construction"
  #pack $pane.label -side top

  #set pane [$nb getframe textfont]
  #label $pane.hello -text "hello\nworld"
  #pack $pane.hello -fill both -expand 1

  #set pane [$nb getframe textstring]
  #text_string_pane $pane

  #set pane [$nb getframe textplaced]
  #text_placed_pane $pane

#  set pane [$nb getframe mixers]
#  button $pane.fizz -text testing

#  set mute 0
#  set scale_w 13
#  set scale_l 150
#  set mixer1 [MakeMixer $pane VHF     $scale_w $scale_l]
#  set mixer2 [MakeMixer $pane Sputnik $scale_w $scale_l]
#  set mixer3 [MakeMixer $pane Hjortoe $scale_w $scale_l]
#  set mixer4 [MakeMixer $pane Comment $scale_w $scale_l]

#  pack $mixer1 -side left -expand false -fill y
#  pack $mixer2 -side left -expand false -fill y
#  pack $mixer3 -side left -expand false -fill y
#  pack $mixer4 -side left -expand false -fill y
#  pack $pane.fizz -fill both -expand true

#  puts "mixer 1 is $mixer1\n"
#  #puts "mixer 1 variable is [$pane.mixer1.scales.scale2 config -variable out2]\n"

  set pane [$nb getframe videoclip]
  videoclip_pane $pane

  set pane [$nb getframe special]
  special_pane $pane

  set pane [$nb getframe config]
  set config_text      "Snowcub version"
  append config_text "\nWindow System"
  append config_text "\nDate"
  append config_text "\nTCL Version"
  #append config_text "\nTk Version"
  foreach name [array names tcl_platform] {
    append config_text "\nPlatform($name)"
  }
  append config_text "\nGstreamer"
  message $pane.config1 -justify left -font fixed -text $config_text -width 400 -bg yellow 

  set config_text      "$snowcub_version"
  append config_text "\n[tk windowingsystem]"
  append config_text "\n[exec date]"
  append config_text "\n[info tclversion]"
  foreach name [array names tcl_platform] {
    append config_text "\n[set tcl_platform($name)]"
  }
  set result "Not Installed"
  catch {set result [exec gstreamer-properties --gst-version] } result"
  append config_text "\n$result"
  message $pane.config2 -justify left -font fixed -text $config_text -width 400 -bg yellow 
  pack $pane.config1 $pane.config2 -side left -expand 0

  pack $nb -fill both -expand 1
  #$nb raise mixers
  #$nb raise audiosink
  #$nb raise audio
  #$nb raise video
  #$nb raise videoclip
  #$nb raise audiofeed
  #$nb raise special
  #$nb raise scenes
  $nb raise $raise_pane
}

proc LoadFeedInfo {} {
  global snowmix feed_info
  puts $snowmix "feed info"
  while {[gets $snowmix line] >= 0} {
    set height 0
    set width 0
    if {[string compare "STAT: " $line] == 0} break
    if {[regexp {STAT:\ +Feed count[\t\ ]+:\ +([0-9]+)} $line match feed_info(feed_count)]} continue
    # STAT: feed id : state islive oneshot geometry cutstart cutsize offset good missed dropped <name>
    # STAT: feed 0 : STALLED recorded continuously 1024x576 0,0 1024x576 0,0 0 0 0 <Internal>
    if {[regexp {STAT:\ +feed ([0-9]+)[\t\ ]+:\ +([^ \t]+)\ +([^ \t]+)\ +([^ \t]+)\ +([0-9]+)x([0-9]+)\ +([0-9]+),([0-9]+)\ +([0-9]+)x([0-9]+)\ +([0-9]+),([0-9]+)\ +([0-9]+)\ +([0-9]+)\ +([0-9]+)\ +\<(.*)\>$} \
       $line match id state islive oneshot width height cutstartx cutstarty cutwidth cutheight offset_x offset_y good missed dropped name]} {

        set feed_info($id,width) $width
        set feed_info($id,height) $height
puts $line
  puts "state $state width $width x $height"
      continue
    }
  }
}

proc LoadSystemInfo {} {
  global snowmix system_info
  puts $snowmix "system info"
  while {[gets $snowmix line] >= 0} {
    if {[string compare "STAT: " $line] == 0} break
    if {[regexp {STAT:\ +System geometry[\t\ ]+:\ +([0-9]+)x([0-9]+)} $line match system_info(width) system_info(height)]} continue
    if {[regexp {STAT:\ +Frame rate[\t\ ]+:\ +([\.0-9]+)} $line match system_info(framerate)]} continue
    if {[regexp {STAT:\ +Stack[\t\ ]+:\ +([0-9 ]+)} $line match system_info(stack)]} continue
puts $line
  }
  puts "Geometry is $system_info(width) x $system_info(height)"
  puts "Frame rate $system_info(framerate)"
  puts "Stack $system_info(stack)"
}

proc Reader { fd } {
	#puts "Fileevent on reader\n"
	if [eof $fd] {
		puts "closing reader\n"
		catch {close $fd}
		return
	}
	#puts "Reading from fd\n"
	gets $fd line
	puts "Read <$line>"
}

proc Writer { fd } {
	global wcount
	incr wcount
	puts "Fileevent on writer $wcount\n"
	if [eof $fd] {
		puts "closing writer\n"
		catch {close $fd}
		return
	}
}

proc get_audio_feeds {} {
	global snowmix
	puts "write to socket\n"
	puts $snowmix "audio feed add"
	puts "written to socket\n"
}

set wcount 1
#set snowmix [socket -async $snowmix_ip $snowmix_port]

proc OpenSocket { ip port } {
  set mysocket [socket $ip $port]
  fconfigure $mysocket -blocking 1 -buffering line
  FileMenuConnect disabled
  return $mysocket
}

set snowmix [OpenSocket $snowmix_ip $snowmix_port]
if {[gets $snowmix line] <0 } {
  puts "failed to get snowmix"
  exit
}

set snowmix2 [socket $snowmix_ip $snowmix_port]
if {[gets $snowmix2 line] <0 } {
  puts "failed to get snowmix2"
  exit
}
fconfigure $snowmix2 -blocking 1 -buffering line
puts $snowmix "audio feed verbose 1"
gets $snowmix line
puts $snowmix "audio mixer verbose 1"
gets $snowmix line
puts $snowmix "audio sink verbose 1"
gets $snowmix line
#fileevent $snowmix readable [list Reader $snowmix]
#fileevent $snowmix writable [list Writer $snowmix]

LoadSystemInfo
#LoadSnowmixInfo $snowmix text
#LoadSnowmixInfo $snowmix font
LoadSnowmixInfo $snowmix image
LoadFeedInfo


#Overview
define_tabs
puts "VOLUME VOL"
#set vol_args [lindex $vol_meter 0]
#set vol [lindex $vol_args 0]
after 1000 StatusSample
puts "VOLUME VOL"

