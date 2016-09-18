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

	var comms = window.comms = {};
	comms.$log = $('#log');

	comms.sendData = function(data) {
		socket.emit('sendData', {data: data });
		comms.$log.prepend(data);
	};

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
		data.push(3);
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