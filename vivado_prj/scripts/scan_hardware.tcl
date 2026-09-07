open_hw_manager
connect_hw_server -url localhost:3121
set script_dir [file normalize [file dirname [info script]]]
set report [open [file join $script_dir .. output hardware_scan.txt] w]
puts "QIMING_HW_SERVERS_BEGIN"
puts [get_hw_servers]
puts $report "HW_SERVERS=[get_hw_servers]"
puts "QIMING_HW_SERVERS_END"
puts "QIMING_HW_TARGETS_BEGIN"
puts [get_hw_targets]
puts $report "HW_TARGETS=[get_hw_targets]"
foreach target [get_hw_targets] {
    puts "TARGET=$target"
    puts $report "TARGET=$target"
    current_hw_target $target
    if {[catch {open_hw_target} error]} {
        puts $report "OPEN_ERROR=$error"
    } else {
        puts "DEVICES=[get_hw_devices]"
        puts $report "DEVICES=[get_hw_devices]"
        foreach device [get_hw_devices] {
            set part [get_property PART $device]
            set idcode [get_property IDCODE $device]
            puts "DEVICE=$device PART=$part IDCODE=$idcode"
            puts $report "DEVICE=$device PART=$part IDCODE=$idcode"
        }
        close_hw_target
    }
}
puts "QIMING_HW_TARGETS_END"
close $report
disconnect_hw_server
close_hw_manager
exit
