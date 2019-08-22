inlets = 3 	// pos, event time, size
outlets = 2	// event index, bang to play

var hop = 512;
var i = 0;
var t = 0;
var size = 0;

if (jsarguments.length>1)
	hop = jsarguments[1];

function bang() {
	i = 0;
	t = 0;
	}
	
function msg_int(r) {
	var ms = r * 1000 * hop / 44100;
	if (inlet == 0) {
		// position
		while(ms >= t && i <= size) {
			outlet(1, bang);
			outlet(0, i);
			i++;
			}			
		}
	if (inlet == 1) {
		// time
		if (r > t)	// future event
			t = r;
		}
	if (inlet == 2) {
		size = r;
		post("there are "+size+" drum events\n");
		}
	}