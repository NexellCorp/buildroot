{
	"result"   : "/tmp/log",
	"continue" : false,

	"0": {
		"descript" : "VPU",
		"thread"   : true,
		"path"	   : "/usr/bin",
		"command"  : "video_api_test",
		"args"     : " -i /mnt/usbdsk/test.mkv -l31 -p30,/mnt/usbdsk/golden/test_golden",
		"logofile" : "log_vpu.txt"
	},
	"1": {
		"descript" : "USB",
		"thread"   : true,
		"path"	   : "/usr/bin",
		"command"  : "disk_test",
		"args"     : " -p /mnt/otg_ums/ -w -r -f 1m -c 10",
		"logofile" : "log_ums.txt"
	}
}
