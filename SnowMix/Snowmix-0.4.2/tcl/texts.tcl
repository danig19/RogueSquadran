proc text_pane { pane } {
  global snowmix textpane
  global text_strings text_fonts

  # Get settings for text
  LoadSnowmixInfo $snowmix { text font }

  # Create a Notebook panel for strings, fonts and placed text
  set textpane [NoteBook $pane.nb -side top -arcradius 4 -homogeneous 1 -tabbevelsize 2]
  $textpane insert 0 strings  -text " Strings"
  $textpane insert end fonts  -text " Fonts"
  $textpane insert end placed -text " Placed Texts"
  pack $textpane -side top -fill x

  set pane [$textpane getframe strings]
  text_string_pane $pane

  set pane [$textpane getframe fonts]
  label $pane.hello -text "hello\nworld"
  pack $pane.hello -fill both -expand 1

  set pane [$textpane getframe placed]
  text_placed_pane $pane

  catch { $textpane raise strings }
}

proc LoadSnowmixInfo { snowmix types } {
  global text_strings text_fonts image_loaded

#puts "================================================"
  foreach type $types {
#puts "TYPE=$type"
    if {[string match text $type]} {
      set command "text string"
      set var "text_strings"
    } elseif {[string match font $type]} {
      set command "text font"
      set var text_fonts
    } elseif {[string match image $type]} {
      set command "image load"
      set var image_loaded
    } elseif {[string match textplaced $type]} {
      set command "text placed info"
      set var text_placed
    }
  
    upvar $var arrayvar
    # Get text strings
    puts $snowmix $command
    if {[string match text $type] || [string match font $type] || [string match image $type]} {
      while {[gets $snowmix line] >= 0} {
#puts "GOT<$line>"
        if {[string compare "STAT: " $line] == 0} {
	    break
        }
        if {[string compare "MSG:" $line] == 0} {
	    break
        }
        set id -1
        regexp {[^0-9]+([0-9]+)\ <(.*)>$} $line match id text
        if { $id > -1 } {
          set arrayvar($id) $text
        }
      }
    } elseif {[string match textplaced $type]} {
      while {[gets $snowmix line] >= 0} {
        if {[string compare "STAT: " $line] == 0} {
	    break
        }
        if {[string compare "MSG:" $line] == 0} {
	    break
        }
        set id -1
      }
      # STAT:  text place id : pos offset wxh align anchor scale rot col clip pad colbg clipbg
      #regexp {[^0-9]+([0-9]+)\ +:\ +([0-9]+)\ ([0-9]+)\ ([\+\-0-9]+)\,([\+\-0-9]+)\ +([0-9]+)\ ([0-9]+)\ } $line match id x  y 
  
    }
  }
}
