connect -url tcp:127.0.0.1:3121
puts "[clock format [clock seconds]] JTAG connected"
set script_dir [file normalize [file dirname [info script]]]
set project_root [file normalize [file join $script_dir .. ..]]

targets -set -nocase -filter {name =~ "APU*"}
rst -system
after 1000

source [file join $project_root vitis_prj workspace qiming_yolo_platform hw ps7_init.tcl]
ps7_init
ps7_post_config
fpga [file join $project_root vivado_prj output design_1_wrapper.bit]

targets -set -nocase -filter {name =~ "ARM Cortex-A9 MPCore #0"}
stop
dow [file join $project_root vitis_prj output yolo_qiming.elf]
puts "LED_DATA_BEFORE=[mrd 0xE000A040]"
puts "LED_DIR_BEFORE=[mrd 0xE000A204]"
puts "LED_OEN_BEFORE=[mrd 0xE000A208]"
con
after 20000
stop
puts "LED_DATA_AFTER=[mrd 0xE000A040]"
puts "LED_READ_AFTER=[mrd 0xE000A060]"
puts "LED_DIR_AFTER=[mrd 0xE000A204]"
puts "LED_OEN_AFTER=[mrd 0xE000A208]"
puts "PC_AFTER=[rrd pc]"
puts "LR_AFTER=[rrd lr]"
con
disconnect
exit
