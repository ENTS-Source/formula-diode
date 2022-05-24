var net = require('net');
var server = net.createServer();
server.on('connection', handleConnection);
server.listen(9000, function () {
    console.log('server listening to %j', server.address());
});
function handleConnection(conn) {
    var remoteAddress = conn.remoteAddress + ':' + conn.remotePort;
    console.log('new client connection from %s', remoteAddress);
    conn.on('data', onConnData);
    conn.once('close', onConnClose);
    conn.on('error', onConnError);
    let cmd = "";
    function onConnData(d) {
        // TODO: Actually read data properly
        console.log(d.length);
        const str = d.toString();
        let i = 0;
        while(!cmd.endsWith('\n') && i < str.length) {
            cmd += str[i++];
        }
        if (cmd.endsWith('\n')) {
            console.log(`Command from ${remoteAddress}: ${cmd.trim()}`);
        }
        // conn.write(d);
    }
    function onConnClose() {
        console.log('connection from %s closed', remoteAddress);
    }
    function onConnError(err) {
        console.log('Connection %s error: %s', remoteAddress, err.message);
    }
}