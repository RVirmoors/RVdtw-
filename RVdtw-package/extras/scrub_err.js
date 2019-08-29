inlets = 4 	// pos, alignment error, event time, score size
outlets = 2	// event index, bang to play

var hop = 512;
var i = 0;
var t = 0;
var size = 40;
var b_err = 0;
var next_t = 0;
var thresh = 3;
var waiting = true;

if (jsarguments.length>1)
	hop = jsarguments[1];

function bang() {
	i = 0;
	t = 0;
	b_err = 0;
	next_t = 0;
	waiting = true;
	}
	
function msg_int(r) {
	var ms = r * 1000 * hop / 44100;
	if (inlet == 0) {
		// position
		if (Math.abs(b_err) < thresh && waiting) {
			t = next_t;
			waiting = false;
			}			
		while(ms >= t && i <= size && !waiting) {
			outlet(1, bang);
			outlet(0, i);
			i++;
			}			
		}
	if (inlet == 1) {
		// back-DTW error
			b_err = r;
		}
	if (inlet == 2) {
		// time
		if (r > t)	{// future event
			next_t = r;
			waiting = true;
			}
		}
	if (inlet == 3) {
		size = r;
		post("there are "+size+" drum events\n");
		}
	}