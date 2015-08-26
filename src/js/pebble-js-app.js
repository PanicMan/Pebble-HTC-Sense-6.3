//Message Queue to send arrays to pebble. https://github.com/smallstoneapps/js-message-queue
var MessageQueue=function(){var RETRY_MAX=5;var queue=[];var sending=false;var timer=null;return{reset:reset,sendAppMessage:sendAppMessage,size:size};function reset(){queue=[];sending=false}function sendAppMessage(message,ack,nack){if(!isValidMessage(message)){return false}queue.push({message:message,ack:ack||null,nack:nack||null,attempts:0});setTimeout(function(){sendNextMessage()},1);return true}function size(){return queue.length}function isValidMessage(message){if(message!==Object(message)){return false}var keys=Object.keys(message);if(!keys.length){return false}for(var k=0;k<keys.length;k+=1){var validKey=/^[0-9a-zA-Z-_]*$/.test(keys[k]);if(!validKey){return false}var value=message[keys[k]];if(!validValue(value)){return false}}return true;function validValue(value){switch(typeof value){case"string":return true;case"number":return true;case"object":if(toString.call(value)=="[object Array]"){return true}}return false}}function sendNextMessage(){if(sending){return}var message=queue.shift();if(!message){return}message.attempts+=1;sending=true;Pebble.sendAppMessage(message.message,ack,nack);timer=setTimeout(function(){timeout()},1e3);function ack(){clearTimeout(timer);setTimeout(function(){sending=false;sendNextMessage()},200);if(message.ack){message.ack.apply(null,arguments)}}function nack(){clearTimeout(timer);if(message.attempts<RETRY_MAX){queue.unshift(message);setTimeout(function(){sending=false;sendNextMessage()},200*message.attempts)}else{if(message.nack){message.nack.apply(null,arguments)}}}function timeout(){setTimeout(function(){sending=false;sendNextMessage()},1e3);if(message.ack){message.ack.apply(null,arguments)}}}}();

var initialised = false;
var transferInProgress = false;
var forecastWeatherFetch = false;
var CityID = 0, posLat = "0", posLon = "0", lang = "en";
//-----------------------------------------------------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------------------------------------------------
var locationOptions = {
	enableHighAccuracy: true, 
	maximumAge: 10000, 
	timeout: 10000
};
//-----------------------------------------------------------------------------------------------------------------------
function locationSuccess(pos) {
	console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);
	posLat = (pos.coords.latitude).toFixed(3);
	posLon = (pos.coords.longitude).toFixed(3);
	
	if (forecastWeatherFetch === false)
		updateWeather();
	else
		updateWeatherForecast();
}
//-----------------------------------------------------------------------------------------------------------------------
function locationError(err) {
	posLat = "0";
	posLon = "0";
	console.log('location error (' + err.code + '): ' + err.message);
}
//-----------------------------------------------------------------------------------------------------------------------
Pebble.addEventListener('ready', 
	function(e) {
		initialised = true;
		var p_lang = "en_US";
		
		//Get pebble language
		if(Pebble.getActiveWatchInfo) {
			try {
				var watch = Pebble.getActiveWatchInfo();
				p_lang = watch.language;
			} catch(err) {
				console.log("Pebble.getActiveWatchInfo(); Error!");
			}
		} 
		
		//Choose language
		var sub = p_lang.substring(0, 2);
		if (sub === "de")
			lang = "de";
		else  if (sub === "es")
			lang = "es";
		else if (sub === "fr")
			lang = "fr";
		else if (sub === "it")
			lang = "it";
		else
			lang = "en";

		console.log("JavaScript app ready and running! Pebble lang: " + p_lang + ", using for Weather: " + lang);
		sendMessageToPebble({"JS_READY": 1});		
	}
);
//-----------------------------------------------------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------------------------------------------------
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
			forecastWeatherFetch = false;
			if (CityID === 0)
				navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
			else
				updateWeather();
		}
		else if ('FC_CKEY' in e.payload) {	//Forecast Weather Download
			CityID = e.payload.FC_CKEY;
			forecastWeatherFetch = true;
			if (CityID === 0)
				navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
			else
				updateWeatherForecast();
		}
    }
);
//-----------------------------------------------------------------------------------------------------------------------
function updateWeather() {
	console.log("Updating weather");
	var req = new XMLHttpRequest();
	var URL = "http://api.openweathermap.org/data/2.5/weather?";
	
	if (CityID !== 0)
		URL += "id="+CityID.toString();
		//https://query.yahooapis.com/v1/public/yql?q=select%20*%20from%20weather.forecast%20where%20u=%22c%22%20and%20woeid=20066914&format=json
	else if (posLat != "0" && posLon != "0")
		URL += "lat=" + posLat + "&lon=" + posLon;
		//https://query.yahooapis.com/v1/public/yql?q=select%20*%20from%20weather.forecast%20where%20u=%22c%22%20uand%20uwoeid%20in%20%28select%20woeid%20from%20geo.placefinder%20where%20text=%2248.062,%208.538%22%20and%20gflags=%22R%22%29&format=json
	else
		return; //Error
	
	URL += "&units=metric&lang=" + lang + "&type=accurate";
	console.log("UpdateURL: " + URL);
	req.open("GET", URL, true);
	req.onload = function(e) {
		if (req.readyState == 4) {
			if (req.status == 200) {
				var response = JSON.parse(req.responseText);
				var temp = Math.round(response.main.temp);//-273.15
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
//-----------------------------------------------------------------------------------------------------------------------
function updateWeatherForecast() {
	console.log("Updating weather forecast");
	var req = new XMLHttpRequest();
	var URL = "http://api.openweathermap.org/data/2.5/forecast/daily?";
	
	if (CityID !== 0)
		URL += "id="+CityID.toString();
	else if (posLat != "0" && posLon != "0")
		URL += "lat=" + posLat + "&lon=" + posLon;
	else
		return; //Error
	
	URL += "&units=metric&lang=" + lang + "&type=accurate&cnt=6";
	console.log("UpdateURL: " + URL);
	req.responseType = "json";
	req.open("GET", URL, true);
	req.onreadystatechange = function(e) {
		if (req.readyState == 4) {
			if (req.status == 200) {
				var response = req.response;//JSON.parse(req.responseText); No Need to parse as this is allready JSON Object
				//console.log("Received Forecast Data: " + JSON.stringify(response)); Crashes if there is an umlaut/unicode
				if ('cnt' in response && response.cnt > 1)
				{
					var message = {};
					for (var i=1; i<response.cnt; i++) {
						var k_dt = "FC_DATE"+i, dt = response.list[i].dt.toString();
						var k_tl = "FC_TEMP_L"+i, temp_l = Math.round(response.list[i].temp.min);
						var k_th = "FC_TEMP_H"+i, temp_h = Math.round(response.list[i].temp.max);
						var k_cd = "FC_COND"+i, cond = response.list[i].weather[0].description;
						var k_ic = "FC_ICON"+i, icon = response.list[i].weather[0].icon;

                        message[k_dt] = dt;
                        message[k_tl] = temp_l;
                        message[k_th] = temp_h;
                        message[k_cd] = cond;
                        message[k_ic] = weatherIconMini[icon];
					}
					MessageQueue.sendAppMessage(message);
				}
				else
					console.log("Count error: " + response.cnt);
			}
			else
				console.log("Status error: " + req.status);
		}
	};
	req.send(null);
}
//-----------------------------------------------------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------------------------------------------------
Pebble.addEventListener('showConfiguration', 
	function(e) {
		var options = JSON.parse(localStorage.getItem('htc_sense_opt'));
		console.log("stored options: " + JSON.stringify(options));
		console.log("showing configuration");

		var uri = 'http://panicman.github.io/config_htcsense.html?title=HTC%20Sense%206.3%20v1.7';
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
				'&units=' + encodeURIComponent(options.units) + 
				'&weather_fc=' + encodeURIComponent(options.weather_fc) + 
				'&weather_aso=' + encodeURIComponent(options.weather_aso) + 
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
				'&vibr_bc=' + encodeURIComponent(options.vibr_bc) + 
				'&debug=' + encodeURIComponent(options.debug) +
				'&hc_mode=' + encodeURIComponent(options.hc_mode);
		}
		console.log("Uri: " + uri);
		Pebble.openURL(uri);
	}
);
//-----------------------------------------------------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------------------------------------------------
