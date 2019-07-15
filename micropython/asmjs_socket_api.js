window.socks = { "cors": null, "name":"websocket","id":-1, "err": {} }


function blob_as_str(b) {
    var u, x
    u = URL.createObjectURL(b)
    x = new XMLHttpRequest()
    x.open('GET', u, false) // although sync, you're not fetching over internet
    x.send()
    URL.revokeObjectURL(u)
    return x.responseText
}



// setup websocket with callbacks
var ws = new WebSocket('ws://localhost:26667/', 'binary')  // or ['binary',]

ws.binaryType = "blob"   // or 'arrayBuffer'

ws.onopen = function() {
    log('CONNECT')
    ws.send( new Blob(["CAP LS\nNICK micropy\nUSER micropy micropy locahost :wsocket\nJOIN #sat\n"]) )
}

ws.onclose = function() { log('DISCONNECT') }

ws.onmessage = function(e) {
    e = blob_as_str(e.data)

    //setTimeout( pump, 5)
    if (e.startsWith("//")) {
    // S == sync call
        if (e.startsWith("//S:")) {
            e = e.substring(4,e.length)
            console.log("EVAL:",e)
            eval(e)
            return
        }
    // A == async call
        if (e.startsWith("//A:")) {
            embed_call( e.substring(4,e.length) )
            return
        }

       //discard //

    } else {
        log('MESSAGE: ' + e.substring(0,20) )
    }
}


