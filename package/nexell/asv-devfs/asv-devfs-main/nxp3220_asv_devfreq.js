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
	},
	"2": {
		"descript" : "I2S",
		"thread"   : true,
		"path"	   : "/usr/bin",
		"command"  : "i2s_test",
		"args"     : " -p 0 -l 1 -T 3",
		"logofile" : "log_i2s.txt"
	},
	"3": {
	 	"descript" : "ETHERNETS",
	 	"thread"   : true,
	 	"path"	   : "/usr/bin",
	 	"command"  : "server",
	 	"args"     : " eth1 &",
	 	"logofile" : "log_server.txt"
	 },
	"4": {
	 	"descript" : "ETHERNETC",
	 	"thread"   : true,
	 	"path"	   : "/usr/bin",
	 	"command"  : "client",
	 	"args"     : " eth0 eth1 1000000 5",
	 	"logofile" : "log_client.txt"
	 }
}
