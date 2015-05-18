var initialised = false;
var transferInProgress = false;
var CityID = 0, posLat = "0", posLon = "0";

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

//-- Get current location: http://forums.getpebble.com/discussion/21755/pebble-js-location-to-url
var locationOptions = {
	enableHighAccuracy: true, 
	maximumAge: 10000, 
	timeout: 10000
};

function locationSuccess(pos) {
	console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);
	posLat = (pos.coords.latitude).toFixed(3);
	posLon = (pos.coords.longitude).toFixed(3);
	updateWeather();
}

function locationError(err) {
	posLat = "0";
	posLon = "0";
	console.log('location error (' + err.code + '): ' + err.message);
}

Pebble.addEventListener('ready', 
	function(e) {
		initialised = true;
		console.log('JavaScript app ready and running!');
		sendMessageToPebble({"JS_READY": 1});		
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
		else if ('W_CKEY' in e.payload) {	//Weather Download
			CityID = e.payload.W_CKEY;
			if (CityID === 0)
				navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
			else
				updateWeather();
		}
    }
);

function updateWeather() {
	console.log("Updating weather");
	var req = new XMLHttpRequest();
	var URL = "http://api.openweathermap.org/data/2.5/weather?";
	
	if (CityID !== 0)
		URL += "id="+CityID.toString();
	else if (posLat != "0" && posLon != "0")
		URL += "lat=" + posLat + "&lon=" + posLon;
	else
		return; //Error
	
	URL += "&unit=metric&lang=de&type=accurate";
	console.log("UpdateURL: " + URL);
	req.open("GET", URL, true);
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

		var uri = 'http://panicman.github.io/config_htcsense.html?title=HTC%20Sense%206.3%20v1.0';
		if (options !== null) {
			uri += 
				'&ampm=' + encodeURIComponent(options.ampm) + 
				'&smart=' + encodeURIComponent(options.smart) + 
				'&firstwd=' + encodeURIComponent(options.firstwd) + 
				'&grid=' + encodeURIComponent(options.grid) + 
				'&invert=' + encodeURIComponent(options.invert) + 
				'&showmy=' + encodeURIComponent(options.showmy) + 
				'&preweeks=' + encodeURIComponent(options.preweeks) + 
				'&weather=' + encodeURIComponent(options.weather) + 
				'&weather_fc=' + encodeURIComponent(options.weather_fc) + 
				'&units=' + encodeURIComponent(options.units) + 
				'&update=' + encodeURIComponent(options.update) + 
				'&cityid=' + encodeURIComponent(options.cityid) + 
				'&col_bg=' + encodeURIComponent(options.col_bg) + 
				'&col_calbg=' + encodeURIComponent(options.col_calbg) + 
				'&col_calgr=' + encodeURIComponent(options.col_calgr) + 
				'&col_caltx=' + encodeURIComponent(options.col_caltx) + 
				'&col_calhl=' + encodeURIComponent(options.col_calhl) + 
				'&col_calmy=' + encodeURIComponent(options.col_calmy) + 
				'&vibr_all=' + encodeURIComponent(options.vibr_all) + 
				'&quietf=' + encodeURIComponent(options.quietf) + 
				'&quiett=' + encodeURIComponent(options.quiett) + 
				'&vibr_hr=' + encodeURIComponent(options.vibr_hr) + 
				'&vibr_bl=' + encodeURIComponent(options.vibr_bl) + 
				'&vibr_bc=' + encodeURIComponent(options.vibr_bc);
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
			Pebble.sendAppMessage(options, 
				function(e) {
					console.log('Successfully delivered message (' + e.payload + ') with transactionId='+ e.data.transactionId);
				},
				function(e) {
					console.log('Unable to deliver message with transactionId=' + e.data.transactionId + ' Error is: ' + e.error.message);
				}
	);
		} else {
			console.log("no options received");
		}
	}
);
