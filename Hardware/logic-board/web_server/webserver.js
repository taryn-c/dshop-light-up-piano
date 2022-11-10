const io = require("socket.io")(3000);

io.on('connection', socket => {
	console.log(socket.id);
	
	socket.on('pythonToServer', (data) => {
		console.log("data received" + JSON.stringify(data));
		io.emit('serverToReact', data);
		console.log("emitted data");
	});
})
