set script_dir [file normalize [file dirname [info script]]]
set vivado_root [file normalize [file join $script_dir ..]]
set project_file [file join $vivado_root acc_project acc_project.xpr]

open_project $project_file
set_property ip_repo_paths [file normalize [file join $vivado_root .. hls_prj]] [current_project]
update_ip_catalog
open_bd_design [file join $vivado_root acc_project acc_project.srcs sources_1 bd design_1 design_1.bd]
validate_bd_design
set ps [get_bd_cells processing_system7_0]
puts "PROJECT_PART=[get_property PART [current_project]]"
puts "IP_REPOSITORY=[get_property ip_repo_paths [current_project]]"
puts "FCLK0_MHZ=[get_property CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ $ps]"
puts "MIO_GPIO_ENABLE=[get_property CONFIG.PCW_GPIO_MIO_GPIO_ENABLE $ps]"
puts "MIO7_IOTYPE=[get_property CONFIG.PCW_MIO_7_IOTYPE $ps]"
close_project
puts "VIVADO_PROJECT_CHECK_COMPLETE"
