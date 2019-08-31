inlets = 4 	// pos, timing swing, event time, score size
outlets = 2	// event index, bang to play

var hop = 512;
var i = 0;
var t = 0;
var size = 40;
var swing = 0;

if (jsarguments.length>1)
	hop = jsarguments[1];

function bang() {
	i = 0;
	t = 0;
	swing = 0;
	}
	
function msg_int(r) {
	if (inlet == 0) {
		// position			
		while(r >= t && i <= size) {
			outlet(1, bang);
			outlet(0, i);
			i++;
			}			
		}
	if (inlet == 1) {
		// swing value
			swing = r;
	//		post(swing);
		}
	if (inlet == 2) {
		// time
		if (r > t)	{// future event
			t = r + swing;
			}
		}
	if (inlet == 3) {
		size = r;
		post("there are "+size+" drum events\n");
		}
	}