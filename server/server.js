const WebSocket = require('ws');

const server = new WebSocket.Server({ host: 'localhost', port: 8765 });

server.on('connection', ws => {
    console.log('New client connected');

    ws.on('message', message => {
        console.log(`Received: ${message}`);
    });

    ws.on('close', () => {
        console.log('Client disconnected');
    });

    ws.on('error', error => {
        console.error(`WebSocket error: ${error}`);
    });

    ws.on('pong', () => {
        console.log('Received pong from client');
    });

    const interval = setInterval(() => {
        if (ws.readyState === WebSocket.OPEN) {
            ws.send('Hello, WebSocket!');
            ws.ping();
        } else {
            clearInterval(interval);
        }
    }, 1000);
});

server.on('listening', () => {
    console.log('WebSocket server started at ws://192.168.1.10:8765');
});

server.on('error', error => {
    console.error(`Server error: ${error}`);
});
