var initialised = false;
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
);

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
