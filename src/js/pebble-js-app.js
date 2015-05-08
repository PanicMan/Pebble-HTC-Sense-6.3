var initialised = false;
var transferInProgress = false;

//http://panicman.github.io/images/weather_big0-12.png
var weatherIcon = {
    "01d" : 0,
    "02d" : 1,
    "03d" : 2,
    "04d" : 3,
    "09d" : 4,
    "10d" : 5,
    "11d" : 6,
    "13d" : 7,
    "50d" : 8,
    "01n" : 9,
    "02n" : 10,
    "03n" : 11,
    "04n" : 3,
    "09n" : 4,
    "10n" : 12,
    "11n" : 6,
    "13n" : 7,
    "50n" : 8
};

//http://panicman.github.io/images/weather_mini0-8.png
var weatherIconMini = {
    "01d" : 0,
    "02d" : 1,
    "03d" : 2,
    "04d" : 3,
    "09d" : 4,
    "10d" : 5,
    "11d" : 6,
    "13d" : 7,
    "50d" : 8,
    "01n" : 0,
    "02n" : 1,
    "03n" : 2,
    "04n" : 3,
    "09n" : 4,
    "10n" : 5,
    "11n" : 6,
    "13n" : 7,
    "50n" : 8
};

Pebble.addEventListener('ready', 
	function(e) {
		initialised = true;
		console.log('JavaScript app ready and running!');
	}
);

function sendMessageToPebble(payload) {
	Pebble.sendAppMessage(payload, 
		function(e) {
			console.log('Successfully delivered message (' + e.payload + ') with transactionId='+ e.data.transactionId);
		},
		function(e) {
			console.log('Unable to deliver message with transactionId=' + e.data.transactionId + ' Error is: ' + e.error.message);
		}
	);
}

Pebble.addEventListener('appmessage',
    function(e) {
		console.log("Got message: " + JSON.stringify(e));

		//Image Download?
		if ('NETDL_URL' in e.payload) {
			if (transferInProgress === false) {
				transferInProgress = true;
				downloadBinaryResource(e.payload.NETDL_URL, 
					function(bytes) {
						transferImageBytes(bytes, e.payload.NETDL_CHUNK_SIZE,
							function(e) { console.log("Done!"); transferInProgress = false; },
							function(e) { console.log("Failed! " + e); transferInProgress = false; }
						);
					},
					function(e) {
						console.log("Download failed: " + e); transferInProgress = false;
					});
			}
			else {
				console.log("Ignoring request to download " + e.payload.NETDL_URL + " because another download is in progress.");
			}
		}
		else {	//Weather Download
			var params=e.payload;
			console.log("updating weather");
			var req = new XMLHttpRequest();
			req.open("GET", "http://api.openweathermap.org/data/2.5/weather?id="+params.W_CKEY.toString()+"&unit=metric&lang=de&type=accurate", true);
			req.onload = function(e) {
				if (req.readyState == 4) {
					if (req.status == 200) {
						var response = JSON.parse(req.responseText);
						var temp = response.main.temp-273.15;
						var cond = response.weather[0].description;
						var icon = response.weather[0].icon;
						var name = response.name;
						sendMessageToPebble({
							"W_TEMP": temp,
							"W_COND": cond,
							"W_ICON": weatherIcon[icon],
							"W_CITY": name
						});
					}
				}
			};
			req.send(null);
		}
    }
);

function downloadBinaryResource(imageURL, callback, errorCallback) {
	var req = new XMLHttpRequest();
	req.open("GET", imageURL,true);
	req.responseType = "arraybuffer";
	req.onload = function(e) {
		console.log("loaded");
		var buf = req.response;
		if(req.status == 200 && buf) {
			var byteArray = new Uint8Array(buf);
			var arr = [];
			for(var i=0; i<byteArray.byteLength; i++) {
				arr.push(byteArray[i]);
			}

			console.log("Downloaded file with " + byteArray.length + " bytes.");
			callback(arr);
		}
		else {
			errorCallback("Request status is " + req.status);
		}
	};
	req.onerror = function(e) {
		errorCallback(e);
	};
	req.send(null);
}

function transferImageBytes(bytes, chunkSize, successCb, failureCb) {
	var retries = 0;
	
	var success = function() {
		console.log("Success cb=" + successCb);
		if (successCb !== undefined) {
			successCb();
		}
	};
	var failure = function(e) {
		console.log("Failure cb=" + failureCb);
		if (failureCb !== undefined) {
			failureCb(e);
		}
	};

	// This function sends chunks of data.
	var sendChunk = function(start) {
		var txbuf = bytes.slice(start, start + chunkSize);
		console.log("Sending " + txbuf.length + " bytes - starting at offset " + start);
		Pebble.sendAppMessage({ "NETDL_DATA": txbuf },
			function(e) {
				// If there is more data to send - send it.
				if (bytes.length > start + chunkSize) {
					sendChunk(start + chunkSize);
				}
				// Otherwise we are done sending. Send closing message.
				else {
					Pebble.sendAppMessage({"NETDL_END": "done" }, success, failure);
				}
			},
			// Failed to send message - Retry a few times.
			function (e) {
				if (retries++ < 3) {
					console.log("Got a nack for chunk #" + start + " - Retry...");
					sendChunk(start);
				}
				else {
					failure(e);
				}
			}
		);
	};

	// Let the pebble app know how much data we want to send.
	Pebble.sendAppMessage({"NETDL_BEGIN": bytes.length },
		function (e) {
			// success - start sending
			sendChunk(0);
		}, 
		failure
	);
}

Pebble.addEventListener('showConfiguration', 
	function(e) {
		var options = JSON.parse(localStorage.getItem('htc_sense_opt'));
		console.log("stored options: " + JSON.stringify(options));
		console.log("showing configuration");

		var uri = 'http://panicman.byto.de/config_htcsense.html?title=HTC%20Sense%206.3%20v1.0';
		if (options !== null) {
			uri += 
				'&theme=' + encodeURIComponent(options.theme) + 
				'&fsm=' + encodeURIComponent(options.fsm) + 
				'&inv=' + encodeURIComponent(options.inv) + 
				'&anim=' + encodeURIComponent(options.anim) + 
				'&sep=' + encodeURIComponent(options.sep) +
				'&datefmt=' + encodeURIComponent(options.datefmt) + 
				'&smart=' + encodeURIComponent(options.smart) + 
				'&vibr=' + encodeURIComponent(options.vibr);
		}
		console.log("Uri: " + uri);
		Pebble.openURL(uri);
	}
);

Pebble.addEventListener('webviewclosed', 
	function(e) {
		console.log("configuration closed");
		if (e.response !== '') {
			var options = JSON.parse(decodeURIComponent(e.response));
			console.log("storing options: " + JSON.stringify(options));
			localStorage.setItem('htc_sense_opt', JSON.stringify(options));
			sendMessageToPebble(options);
		} else {
			console.log("no options received");
		}
	}
);
