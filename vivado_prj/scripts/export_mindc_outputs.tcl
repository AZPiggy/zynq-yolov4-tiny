set root {C:/Users/23690/Desktop/zynq_yolo/yolov4_tiny_acc_accelerator/vivado_prj}
set run_dir [file join $root acc_project acc_project.runs impl_1]
set out [file join $root output]
file mkdir $out
open_checkpoint [file join $run_dir design_1_wrapper_routed.dcp]
report_timing_summary -file [file join $out timing_summary_mindc.rpt]
report_utilization -file [file join $out utilization_mindc.rpt]
write_bitstream -force [file join $out design_1_wrapper_mindc.bit]
write_hw_platform -fixed -include_bit -force -file [file join $out mindc_yolo.xsa]
puts "MINDC_EXPORT_COMPLETE"
