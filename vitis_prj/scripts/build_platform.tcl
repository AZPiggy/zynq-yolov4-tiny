set script_dir [file normalize [file dirname [info script]]]
set vitis_root [file normalize [file join $script_dir ..]]
set project_root [file normalize [file join $vitis_root ..]]
set build_ws [file join $vitis_root workspace_mindc]
set xsa [file join $project_root vivado_prj output mindc_yolo.xsa]

proc patch_conv_driver_makefiles {build_ws} {
    foreach makefile [list \
        [file join $build_ws mindc_yolo_platform ps7_cortexa9_0 standalone_domain bsp ps7_cortexa9_0 libsrc conv_v0_0 src Makefile] \
        [file join $build_ws mindc_yolo_platform zynq_fsbl zynq_fsbl_bsp ps7_cortexa9_0 libsrc conv_v0_0 src Makefile]] {
        if {[file exists $makefile]} {
            set input [open $makefile r]
            set contents [read $input]
            close $input
            set contents [string map {"LIBSOURCES=*.c" "LIBSOURCES=xconv.c xconv_sinit.c xconv_g.c" "OUTS = *.o" "OUTS=xconv.o xconv_sinit.o xconv_g.o"} $contents]
            set output [open $makefile w]
            puts -nonewline $output $contents
            close $output
        }
    }
}

file mkdir $build_ws
cd $build_ws
setws .
if {[file exists [file join $build_ws mindc_yolo_platform platform.spr]]} {
    platform active mindc_yolo_platform
    platform config -updatehw $xsa
} else {
    platform create -name mindc_yolo_platform -hw $xsa -proc ps7_cortexa9_0 -os standalone
}
domain active standalone_domain
bsp setlib -name xilffs
bsp config use_lfn 2
bsp config stdin ps7_uart_0
bsp config stdout ps7_uart_0
bsp write

# The standalone BSP copy appears only during the first generation attempt.
patch_conv_driver_makefiles $build_ws
if {[catch {platform generate} generation_error]} {
    puts "Retrying after patching the generated HLS 2019.2 driver Makefile"
    patch_conv_driver_makefiles $build_ws
    platform generate
}
puts "MINDC_VITIS_PLATFORM_COMPLETE"
