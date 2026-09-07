set script_dir [file normalize [file dirname [info script]]]
set vivado_root [file normalize [file join $script_dir ..]]
set project_root [file normalize [file join $vivado_root ..]]
set project_file [file join $vivado_root acc_project acc_project.xpr]
set output_dir [file join $vivado_root output]
file mkdir $output_dir

open_project $project_file
set_property ip_repo_paths [file join $project_root hls_prj top top solution1 impl ip] [current_project]
update_ip_catalog
open_bd_design [file join $vivado_root acc_project acc_project.srcs sources_1 bd design_1 design_1.bd]
report_ip_status -file [file join $output_dir ip_status_before_upgrade.rpt]
set locked_ips [get_ips -all -filter {IS_LOCKED == 1}]
if {[llength $locked_ips] > 0} {
    upgrade_ip $locked_ips
}
set ps [get_bd_cells processing_system7_0]

# Board parameters are copied from the locally verified ALIENTEK Qiming Zynq-7020 project.
# SD0 card-detect corrected for Mind-Dev-Z7020-V1.1.0: the carrier board's core_connects
# sheet maps B0_MIO9 -> SD_CD and B0_MIO10 -> OTG_RESETN (see Mind-Dev-Z7020-SCH_V1.1.0.pdf,
# page "core_connects"). MIO10 has no relation to the SD socket on this board.
set_property -dict [list \
    CONFIG.PCW_UIPARAM_DDR_PARTNO {MT41K256M16TW-107:P} \
    CONFIG.PCW_UIPARAM_DDR_FREQ_MHZ {533.333333} \
    CONFIG.PCW_UIPARAM_DDR_BUS_WIDTH {32 Bit} \
    CONFIG.PCW_UIPARAM_DDR_BOARD_DELAY0 {0.25} \
    CONFIG.PCW_UIPARAM_DDR_BOARD_DELAY1 {0.25} \
    CONFIG.PCW_UIPARAM_DDR_BOARD_DELAY2 {0.25} \
    CONFIG.PCW_UIPARAM_DDR_BOARD_DELAY3 {0.25} \
    CONFIG.PCW_UIPARAM_DDR_DQS_TO_CLK_DELAY_0 {0.0} \
    CONFIG.PCW_UIPARAM_DDR_DQS_TO_CLK_DELAY_1 {0.0} \
    CONFIG.PCW_UIPARAM_DDR_DQS_TO_CLK_DELAY_2 {0.0} \
    CONFIG.PCW_UIPARAM_DDR_DQS_TO_CLK_DELAY_3 {0.0} \
    CONFIG.PCW_UIPARAM_DDR_T_RC {48.75} \
    CONFIG.PCW_UIPARAM_DDR_USE_INTERNAL_VREF {0} \
    CONFIG.PCW_GPIO_MIO_GPIO_ENABLE {1} \
    CONFIG.PCW_GPIO_MIO_GPIO_IO {MIO} \
    CONFIG.PCW_MIO_7_IOTYPE {LVCMOS 3.3V} \
    CONFIG.PCW_MIO_7_SLEW {slow} \
    CONFIG.PCW_SD0_GRP_CD_IO {MIO 9} \
    CONFIG.PCW_MIO_9_PULLUP {enabled} \
    CONFIG.PCW_MIO_40_PULLUP {enabled} \
    CONFIG.PCW_MIO_40_SLEW {slow} \
    CONFIG.PCW_MIO_41_PULLUP {enabled} \
    CONFIG.PCW_MIO_41_SLEW {slow} \
    CONFIG.PCW_MIO_42_PULLUP {enabled} \
    CONFIG.PCW_MIO_42_SLEW {slow} \
    CONFIG.PCW_MIO_43_PULLUP {enabled} \
    CONFIG.PCW_MIO_43_SLEW {slow} \
    CONFIG.PCW_MIO_44_PULLUP {enabled} \
    CONFIG.PCW_MIO_44_SLEW {slow} \
    CONFIG.PCW_MIO_45_PULLUP {enabled} \
    CONFIG.PCW_MIO_45_SLEW {slow} \
    CONFIG.PCW_UART0_PERIPHERAL_ENABLE {1} \
    CONFIG.PCW_UART0_UART0_IO {MIO 14 .. 15} \
    CONFIG.PCW_UART1_PERIPHERAL_ENABLE {0} \
    CONFIG.PCW_UART0_GRP_FULL_ENABLE {0} \
    CONFIG.PCW_MIO_14_DIRECTION {inout} \
    CONFIG.PCW_MIO_14_IOTYPE {LVCMOS 3.3V} \
    CONFIG.PCW_MIO_14_PULLUP {enabled} \
    CONFIG.PCW_MIO_14_SLEW {slow} \
    CONFIG.PCW_MIO_15_DIRECTION {inout} \
    CONFIG.PCW_MIO_15_IOTYPE {LVCMOS 3.3V} \
    CONFIG.PCW_MIO_15_PULLUP {enabled} \
    CONFIG.PCW_MIO_15_SLEW {slow}] $ps

if {[get_property CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ $ps] ne "120.000000"} {
    error "FCLK_CLK0 changed unexpectedly"
}
validate_bd_design
save_bd_design
generate_target all [get_files design_1.bd]
update_compile_order -fileset sources_1

reset_run synth_1
launch_runs impl_1 -to_step write_bitstream -jobs 4
wait_on_run impl_1
if {[get_property STATUS [get_runs impl_1]] ne "write_bitstream Complete!"} {
    error "Implementation did not complete"
}

open_run impl_1
report_timing_summary -file [file join $output_dir timing_summary.rpt]
report_utilization -file [file join $output_dir utilization.rpt]
write_hw_platform -fixed -include_bit -force -file [file join $output_dir qiming_yolo.xsa]
file copy -force [file join [get_property DIRECTORY [get_runs impl_1]] design_1_wrapper.bit] \
    [file join $output_dir design_1_wrapper.bit]

# Mirror the same build under the mindc_* names consumed by build_platform.tcl / build_boot.ps1,
# so the mindc pipeline always matches what was just synthesized here (no separate re-export step
# that could drift from this hardware, as previously happened with rebuild_mindc_direct.tcl).
report_timing_summary -file [file join $output_dir timing_summary_mindc.rpt]
report_utilization -file [file join $output_dir utilization_mindc.rpt]
write_hw_platform -fixed -include_bit -force -file [file join $output_dir mindc_yolo.xsa]
file copy -force [file join [get_property DIRECTORY [get_runs impl_1]] design_1_wrapper.bit] \
    [file join $output_dir design_1_wrapper_mindc.bit]

close_project
puts "QIMING_BUILD_COMPLETE"
