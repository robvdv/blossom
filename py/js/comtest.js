$(document).ready(function () {
	var comtest = window.comtest = {};

	// Use a "/test" namespace.
	// An application can open a connection on multiple namespaces, and
	// Socket.IO will multiplex all those connections on a single
	// physical channel. If you don't care about multiple channels, you
	// can set the namespace to an empty string.
	namespace = '/test';

	// Connect to the Socket.IO server.
	// The connection URL has the following format:
	//     http[s]://<domain>:<port>[/<namespace>]
	var socket = io.connect('http://' + document.domain + ':' + location.port + namespace);

	socket.on('message', function (data) {
		comms.$log.val(data);
	});

	var comms = window.comms = {};
	comms.$log = $('#log');
	window.lastSentId = 0;

	comms.sendData = function(data) {
		window.lastSentId++;
		if (window.lastSentId > 255) {
			window.lastSentId = 0;
		}
		var sendData = data.slice(0);
		sendData.splice(2,0,window.lastSentId);
		//console.log("sending: " + JSON.stringify(sendData));
		socket.emit('sendData', {data: sendData });
	};

	comms.getStats = window.setInterval(function () {
		comms.sendData([3,0]);
	}, 1000);
	//sending: [5,5,74,0,1,2,3,4]

	$('#send-arbitrary-data').on('click', function() {
		var data = [];
		var dataString = $('#arbitrary-data').val().split(',');
		for (var i=0; i < dataString.length; i++) {
			data.push(parseInt(dataString[i]));
		}
		comms.sendData(data);
	});

	$('#send-testdata-stop').on('click', function() {
		clearInterval(comms.sendInterval);
	});


	$('#send-testdata').on('click', function() {
		var $testData = $('#testdata-value');
		var $msdelay = $('#testdata-msdelay');
		var $dataLength = $('#testdata-length');
		var data = [];
		var len = parseInt($dataLength.val());
		var ms = parseInt($msdelay.val());
		data.push(5);
		data.push(len);
		for (var i = 0; i < len; i++) {
			data.push(i);
		}
		$testData.val(JSON.stringify(data));
		comms.sendInterval = setInterval(
			function() {
				comms.sendData(data);
			},
			ms
		);
	});

	$('#led-on').on('click', function() {
		comms.sendData([1,0]);
	});

	$('#led-off').on('click', function() {
		comms.sendData([2,0]);
	});

});