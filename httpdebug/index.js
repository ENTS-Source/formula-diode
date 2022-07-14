var http = require('http');

http.createServer(function (req, res) {
    console.log(`${req.method} ${req.url}`);
    res.writeHead(200, {'Content-Type': 'text/plain'});
    res.end("OK");
}).listen(20304);
