$(document).ready(function () {
	var blossom = window.blossom = {};

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

	// Event handler for new connections.
	// The callback function is invoked when a connection with the
	// server is established.
	socket.on('connect', function () {
		socket.emit('my event', {data: 'I\'m connected!'});
	});

	// Event handler for server sent data.
	// The callback function is invoked whenever the server emits data
	// to the client. The data is then displayed in the "Received"
	// section of the page.
	socket.on('my response', function (msg) {
		$('#log').append('<br>' + $('<div/>').text('Received #' + msg.count + ': ' + msg.data).html());
	});

	// Interval function that tests message latency by sending a "ping"
	// message. The server then responds with a "pong" message and the
	// round trip time is measured.
	var ping_pong_times = [];
	var start_time;
	window.setInterval(function () {
		start_time = (new Date).getTime();
		socket.emit('my ping');
	}, 1000);

	window.setInterval(function () {
		socket.emit('getMessages');
	}, 1000);

	// Handler for the "pong" message. When the pong is received, the
	// time from the ping is stored, and the average of the last 30
	// samples is average and displayed.
	socket.on('my pong', function () {
		var latency = (new Date).getTime() - start_time;
		ping_pong_times.push(latency);
		ping_pong_times = ping_pong_times.slice(-30); // keep last 30 samples
		var sum = 0;
		for (var i = 0; i < ping_pong_times.length; i++)
			sum += ping_pong_times[i];
		$('#ping-pong').text(Math.round(10 * sum / ping_pong_times.length) / 10);
	});

	var comms = window.comms = {};

	comms.lastSentMs = 0;
	comms.sendThrottleMs = 20;
	comms.sendFrameData = false;

	comms.sendPoints = function() {
		if ((new Date().getTime()) < (comms.lastSentMs + comms.sendThrottleMs)) {
			return;
		}
		comms.lastSentMs = new Date().getTime();
		var data = grapher.createPoints();
		socket.emit('points', {data: data });
		console.log(JSON.stringify({data: [data] }))
	};

	// test!
	comms.sendWave = function() {
		if ((new Date().getTime()) < (comms.lastSentMs + comms.sendThrottleMs)) {
			return;
		}
		comms.lastSentMs = new Date().getTime();
		var data = [[0, 0, 2, 4, grapher.wave.length]];
		socket.emit('points', {data: data });
		console.log(JSON.stringify({data: [data] }))
	};

	comms.arduinoOnOff = function(turnOn) {
		if ((new Date().getTime()) < (comms.lastSentMs + comms.sendThrottleMs)) {
			return;
		}
		comms.lastSentMs = new Date().getTime();
		comms.sendFrameData = turnOn ? 1 : 0;
		var message = [0, 0, 1, 0];
		message.push( comms.sendFrameData );
		socket.emit('points', {data: [message] }); // rename points to frame or something
		console.log(JSON.stringify({data: [message] }))
	};

	socket.on('message', function (msg) {
		if (msg) {
			$('#messages').prepend(msg+ '\n');
		}
	});


	var pointPattern = blossom.pointPattern = {};
	pointPattern.init = function() {
		pointPattern.$canvas = $('#pointPattern');
		pointPattern.$pointsText = $('#points-text'); // split this out
		pointPattern.$waveText = $('#wave-text'); // split this out
		pointPattern.$toneFrequency = $('#tone-frequency'); // split this out
		pointPattern.canvas = pointPattern.$canvas[0];
		pointPattern.context = pointPattern.canvas.getContext("2d");
	};

	pointPattern.clear = function() {
		pointPattern.context.clearRect(0, 0, pointPattern.canvas.width, pointPattern.canvas.height);
	};

	pointPattern.init();

	var wavePattern = blossom.wavePattern = {};
	wavePattern.init = function() {
		wavePattern.$canvas = $('#wavePattern');
		wavePattern.canvas = wavePattern.$canvas[0];
		wavePattern.context = wavePattern.canvas.getContext("2d");
	};

	wavePattern.clear = function() {
		wavePattern.context.clearRect(0, 0, wavePattern.canvas.width, wavePattern.canvas.height);
	};

	wavePattern.init();

	var grapher = blossom.grapher = {};

	grapher.redMask = 1;
	grapher.greenMask = 2;
	grapher.blueMask = 4;

	grapher.init = function(pointPattern, wavePattern) {
		grapher.contextPoints = pointPattern.context;
		grapher.widthPoints = pointPattern.$canvas.width();
		grapher.heightPoints = pointPattern.$canvas.height();

		grapher.contextWaves = wavePattern.context;
		grapher.widthWaves = wavePattern.$canvas.width();
		grapher.heightWaves = wavePattern.$canvas.height();

		grapher.centerX = grapher.widthPoints / 2;
		grapher.centerY = grapher.heightPoints / 2;
		grapher.multX = grapher.centerX; // amount of space between center and boundary (sin/cos multiplier)
		grapher.multY = grapher.centerY;
		grapher.configFrame = [2]; // 2 byte representation of the tone
		grapher.points = []; // calculated points in frames
		grapher.pointsPerFrame = 0;
	};

	grapher.calcCycle = function( tone, toneFrequencyRatio, toneFrequencyOffset, toneAmplitude, pointsPerFrame, circleSize, circleRatio, smallCirclePhase, redMin, redMax, greenMin, greenMax, blueMin, blueMax ) {
		// push in the tone/frame start config
		grapher.points = [1];
		grapher.soundWave = [];

		// push in the points data
		grapher.pointsPerFrame = pointsPerFrame;
		var frameMult = 360 / pointsPerFrame;
		var bigCircleMultiplier = (circleRatio / 100);
		var sizeMultiplier = (circleSize / 100);

		var redMinCutoff = (redMin) / 100;
		var redMaxCutoff = (redMax) / 100;
		var greenMinCutoff = (greenMin) / 100;
		var greenMaxCutoff = (greenMax) / 100;
		var blueMinCutoff = (blueMin) / 100;
		var blueMaxCutoff = (blueMax) / 100;

		for (var i = 0; i < pointsPerFrame; i++) {
			var degrees = frameMult * i;
			var radians = degrees * (Math.PI / 180);

			var bigCircleX = Math.sin(radians) * bigCircleMultiplier;
			var bigCircleY = Math.cos(radians) * bigCircleMultiplier;
			var smallCircleXSin = Math.sin(radians * smallCirclePhase);
			var smallCircleYSin = Math.cos(radians * smallCirclePhase);
			var smallCircleX = smallCircleXSin * (1 - bigCircleMultiplier);
			var smallCircleY = smallCircleYSin * (1 - bigCircleMultiplier);

			var circleSumX = (bigCircleX + smallCircleX);
			var circleSumY = (bigCircleY + smallCircleY);

			var posX = circleSumX * grapher.multX * sizeMultiplier + grapher.centerX;
			var posY = circleSumY * grapher.multY * sizeMultiplier + grapher.centerY;

			var hypotenuse = Math.pow( Math.pow(circleSumX, 2) + Math.pow(circleSumY, 2), 0.5 );

			var color = '#';
			var red = ((hypotenuse >= redMinCutoff) && (hypotenuse <= redMaxCutoff)) ? grapher.redMask : 0;
			var green = ((hypotenuse >= greenMinCutoff) && (hypotenuse <= greenMaxCutoff)) ? grapher.greenMask : 0;
			var blue = ((hypotenuse >= blueMinCutoff) && (hypotenuse <= blueMaxCutoff)) ? grapher.blueMask : 0;

			grapher.points[i] = [Math.floor(posX), Math.floor(posY), red + green + blue];
		}

		var wavePatternOffset = 0; // adjust for offsets later
		var wavePattenCycleRatio = toneFrequencyRatio / 8; // number of point patterns to draw per wave cycle
		var waveBytes = (pointsPerFrame * wavePattenCycleRatio) + wavePatternOffset;

		for (var i = 0; i < waveBytes; i++) {
			var radians = i * (Math.PI / 180) * (360 / waveBytes);
			var amplitudeMultiplier = 1;
			var waveAmplitude = (Math.sin(radians) + 1) / 2 * amplitudeMultiplier; // 0 - 2 range
			grapher.soundWave[i] = Math.floor(waveAmplitude * 255 * toneAmplitude / 100);
		}

	};

	grapher.drawCycle = function() {
		var points = grapher.points;
		var pointsPerFrame = grapher.pointsPerFrame;
		var context = grapher.contextPoints;
		var fromX, fromY, lastX, lastY;
		var red, green, blue;

		// ignore the first config point
		for (var i = 0; i < pointsPerFrame; i++) {
			if (i === 0) {
				fromX = points[pointsPerFrame - 1][0];
				fromY = points[pointsPerFrame - 1][1];
			} else {
				fromX = points[i - 1][0];
				fromY = points[i - 1][1];
			}
			context.beginPath();
			context.moveTo(fromX, fromY);
			context.lineTo(points[i][0], points[i][1]);
			red = (grapher.redMask & points[i][2]) * 255;
			green = (grapher.greenMask & points[i][2]) * 255;
			blue = (grapher.blueMask & points[i][2]) * 255;
			context.strokeStyle = 'rgb(' + red + ','  + green + ','  + blue + ')';
			context.stroke();
		}
	};


	grapher.drawWavePattern = function() {
		var wave = grapher.soundWave;
		var points = grapher.points;
		var pointsPerFrame = grapher.pointsPerFrame;

		var context = grapher.contextWaves;
		var canvasWidth = grapher.widthWaves;
		var canvasHeight = grapher.heightWaves;
		var pointPos, pointPosNext;

		for (var i = 0; i <= canvasWidth; i++) {
			// draw the sound wave
			var waveArrayPos = Math.floor(i * (wave.length / canvasWidth));
			context.beginPath();
			context.moveTo(i, canvasHeight);
			context.lineTo(i, grapher.heightWaves - wave[waveArrayPos]);
			context.strokeStyle = 'rgb(0,0,0)';
			context.stroke();

			// each byte of sound wave data is equal to 8 points
			pointPos = ((waveArrayPos * 8) + 1) % pointsPerFrame;
			pointPosNext = (((waveArrayPos + 1) * 8) + 1) % pointsPerFrame;
			var yPosFrom, yPosTo, xPosFrom, xPosTo;
			// draw the galvo/point pattern
			yPosFrom = points[pointPos][2];
			yPosTo = points[pointPosNext][2];
			xPosFrom = points[pointPos][1];
			xPosTo = points[pointPosNext][1];

			context.beginPath();
			context.moveTo(i, yPosFrom);
			context.lineTo(i, yPosTo);
			red = (grapher.redMask & points[pointPos][2]) * 255;
			green = (grapher.greenMask & points[pointPos][2]) * 255;
			blue = (grapher.blueMask & points[pointPos][2]) * 255;
			context.strokeStyle = 'rgb(' + red + ','  + green + ','  + blue + ')';

			context.stroke();
			context.beginPath();
			context.moveTo(i, xPosFrom);
			context.lineTo(i, xPosTo);
			red = (grapher.redMask & points[pointPos][2]) * 255;
			green = (grapher.greenMask & points[pointPos][2]) * 255;
			blue = (grapher.blueMask & points[pointPos][2]) * 255;
			context.strokeStyle = 'rgb(' + red + ','  + green + ','  + blue + ')';
			context.stroke();

		}
	};

	grapher.updateStats = function() {
		var freq = 31372 / grapher.soundWave.length;
		pointPattern.$toneFrequency.text( freq );
	};

	grapher.clear = function() {
		grapher.contextPoints.clearRect(0, 0, grapher.widthPoints, grapher.heightPoints );
		grapher.contextWaves.clearRect(0, 0, grapher.widthWaves, grapher.heightWaves );
	};

	// unused
	grapher.toBytesInt16 = function(num) {
		return [(num & 0x0000ff00) >> 8, (num & 0x000000ff)];
	};

	grapher.createPoints = function() {
		var arr = [[0,0,2,0,grapher.points.length]];
		return arr.concat(grapher.points);
	};
	grapher.writePoints = function() {
		pointPattern.$pointsText.text( JSON.stringify(grapher.createPoints()) );
	};

	grapher.writeTone = function() {
		var text = '';
		text += '[0,0,3,xxx,'  + grapher.soundWave.length + ']';
		text += '[' + grapher.soundWave[0];
		for (var i = 1; i < grapher.soundWave.length; i++) {
			text += ',' + grapher.soundWave[i];
		}
		text += ']';
		pointPattern.$waveText.text( text );
	};

	grapher.init(pointPattern, wavePattern);

	function drawCircle() {
		//frames, circleSize, circleRatio, smallCirclePhase, redPhase, greenPhase, bluePhase
		grapher.clear();
		grapher.calcCycle(
			sliders['tone'].value,
			sliders['tone-frequency-ratio'].value,
			sliders['tone-frequency-point-offset'].value,
			sliders['tone-amplitude'].value,
			sliders['points-per-frame'].value,
			sliders['circle-size'].value,
			sliders['circle-ratio'].value,
			sliders['circle-size-small'].value,
			sliders['phase-red'].value,
			sliders['phase-red'].value2,
			sliders['phase-green'].value,
			sliders['phase-green'].value2,
			sliders['phase-blue'].value,
			sliders['phase-blue'].value2
		);
		grapher.drawCycle();
		grapher.drawWavePattern();
		grapher.writePoints();
		grapher.writeTone();
		grapher.updateStats();
		if (comms.sendFrameData) {
			comms.sendPoints();
		}
	}

	var changeSlider = function (slider) {
		var newVal = slider.$el.val().split(',');

		slider.$val.text(newVal);
		slider.value = newVal[0];
		slider.value2 = newVal[1];
		drawCircle();
	};

	var sliders = blossom.sliders = {};

	$('input.slider').each(function () {
		var $slider = $(this);
		var $val = $slider.closest('.row').find('.slider-val');
		var sliderOptions = {
			$el: $slider,
			$val: $val,
			value: -1,
			value2: -1
		};
		sliders[$slider.attr('id')] = sliderOptions;
		$slider.slider({
			tooltip: 'hide'
		}).on('slide', function(event) { changeSlider(sliderOptions) });
	});

	$('input.slider').each(function () {
		$(this).trigger('slide');
	});

	var controls = blossom.controls = {};

	controls.$arduinoOnOff = $('#arduino-on-off');
	controls.$arduinoOnOff.on('change', function() {
		comms.arduinoOnOff( controls.$arduinoOnOff.is(':checked') );
	});



});