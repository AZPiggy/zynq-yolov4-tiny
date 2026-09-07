set root {C:/Users/23690/Desktop/zynq_yolo/yolov4_tiny_acc_accelerator/vivado_prj}
set proj [file join $root acc_project]
set src [file join $proj acc_project.srcs sources_1 bd design_1]
set out [file join $root output]
file mkdir $out
create_project -in_memory -part xc7z020clg400-2
set_property ip_repo_paths {C:/Users/23690/Desktop/zynq_yolo/yolov4_tiny_acc_accelerator/hls_prj/top/top/solution1/impl/ip} [current_project]
update_ip_catalog
read_verilog -library xil_defaultlib [file join $src hdl design_1_wrapper.v]
add_files [file join $src design_1.bd]
generate_target all [get_files [file join $src design_1.bd]]
read_xdc [file join $proj acc_project.runs synth_1 dont_touch.xdc]
synth_design -top design_1_wrapper -part xc7z020clg400-2
write_checkpoint -force [file join $out mindc_synth.dcp]
opt_design
place_design
phys_opt_design
route_design
report_timing_summary -file [file join $out timing_summary_mindc.rpt]
report_utilization -file [file join $out utilization_mindc.rpt]
write_bitstream -force [file join $out design_1_wrapper_mindc.bit]
write_hw_platform -fixed -include_bit -force -file [file join $out mindc_yolo.xsa]
puts "MINDC_REBUILD_COMPLETE"
