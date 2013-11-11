proc ImageRotation { place_id rotation } {
  return "image place rotation $place_id $rotation\n"
}
proc SetTime {} {
  set sec_list ""
  set a [clock format [clock seconds] -format "%I %M %S"]
  set hour [lindex $a 0]
  set minute [lindex $a 1]
  set secs [lindex $a 2]
  set pi [expr 2*asin(1.0)]
  set rotation_secs [expr $secs * 2 * $pi / 60.0]
  set rotation_minute [expr $minute * 2 * $pi / 60.0]
  set rotation_hour [expr ((60.0*$hour + $minute) / 60.0) * 2 * $pi / 12.0]
  append sec_list "# Time = $hour:$minute:$secs\n"
  for {set i 0} {$i < $secs} {incr i 1} {
    append sec_list ClockSeconds\n
  }
  append sec_list [ImageRotation 12 $rotation_hour]
  append sec_list [ImageRotation 11 $rotation_minute]
  append sec_list [ImageRotation 13 $rotation_secs]
  return $sec_list
}

puts [SetTime]
