{
	"result"   : "/tmp/log",
	"continue" : false,

	"0": {
		"descript" : "disk test mmc0",
		"thread"   : true,
		"path"	   : "/usr/bin",
		"command"  : "disk_test -p /mnt/mmc0 -b 1m -f 15m -c 1 -l 5 -r -w -v",
		"logofile" : "log.txt"
	},

	"1": {
		"descript" : "disk test mmc1",
		"thread"   : true,
		"path"	   : "/usr/bin",
		"command"  : "disk_test -p /mnt/mmc1 -b 1m -f 15m -c 1 -l 5 -r -w -v",
		"logofile" : "log.txt"
	},

	"2": {
		"descript" : "Disk test MMC2",
		"thread"   : true,
		"path"	   : "/usr/bin",
		"command"  : "disk_test -p /mnt/mmc2 -b 1M -f 15M -c 1 -l 5 -r -w -v",
		"logofile" : "log.txt"
	},

	"3": {
		"descript" : "Disk test EHCI0",
		"thread"   : true,
		"path"	   : "/usr/bin",
		"command"  : "disk_test -p /mnt/ehci0 -b 1M -f 15M -c 1 -l 5 -r -w -v",
		"logofile" : "log.txt"
	},

	"4": {
		"descript" : "Disk test EHCI1",
		"thread"   : true,
		"path"	   : "/usr/bin",
		"command"  : "disk_test -p /mnt/ehci1 -b 1M -f 15M -c 1 -l 2 -r -w -v",
		"logofile" : "log.txt"
	},

	"5": {
		"descript" : "VPU",
		"thread"   : true,
		"path"	   : "/usr/bin",
                "command"  : "nx_vpu_test",
                "args"     : "-s 1920,1088 -i /data/bitstream -g /data/golden -k 4 -t 15000 -p 2500",
                "logofile" : "log.txt"
	},

	"6": {
		"descript" : "speaker-test 0,0",
		"thread"   : true,
		"path"	   : "/usr/bin",
		"command"  : "speaker-test -D plughw:0,0 -tw -l 9",
		"logofile" : "log.txt"
	},

	"7": {
		"descript" : "speaker-test 1,0",
		"thread"   : true,
		"path"	   : "/usr/bin",
		"command"  : "speaker-test -D plughw:1,0 -tw -l 9",
		"logofile" : "log.txt"
	},

	"8": {
		"descript" : "speaker-test 2,0",
		"thread"   : true,
		"path"	   : "/usr/bin",
		"command"  : "speaker-test -D plughw:2,0 -tw -l 9",
		"logofile" : "log.txt"
	},

	"9": {
		"descript" : "speaker-test 3,0",
		"thread"   : true,
		"path"	   : "/usr/bin",
		"command"  : "speaker-test -D plughw:3,0 -tw -l 9",
		"logofile" : "log.txt"
	},

	"10": {
		"descript" : "ethernet roofback server",
		"thread"   : true,
		"path"	   : "/usr/bin",
		"command"  : "server eth1",
		"logofile" : "log.txt"
	},

	"11": {
		"descript" : "ethernet roofback client",
		"thread"   : true,
		"path"	   : "/usr/bin",
		"command"  : "client eth0 eth1 1000000 10",
		"logofile" : "log.txt"
	},

	"12": {
		"descript" : "memtester 0",
		"thread"   : true,
		"path"	   : "/usr/bin",
		"command"  : "memtester 200K 6",
		"logofile" : "log.txt"
	},

	"13": {
		"descript" : "memtester 1",
		"thread"   : true,
		"path"	   : "/usr/bin",
		"command"  : "memtester 200K 6",
		"logofile" : "log.txt"
	}
}
