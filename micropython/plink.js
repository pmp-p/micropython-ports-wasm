document.getElementById('test').textContent = "THIS IS A TEST BLOCK\n"
document.title="THIS IS A TEST TITLE"

// ================= (c/up)link =================================================
window.embed = {}
window.embed.state = {}
window.embed.ref = []

function ID(){
     return 'js|' + Math.random().toString(36).substr(2, 9);
}



function embed_call_impl(callid, fn, owner, params) {
    var rv = null;
    try {
        rv = fn.apply(owner,params)
    } catch(x){
        console.log("call failed : "+fn+"("+params+") : "+ x )
    }
    if ( (rv !== null) && (typeof rv === 'object')) {
        var seen = false
        var rvid = null;
        for (var i=0;i<window.embed.ref.length;i++) {
            if ( Object.is(rv, window.embed.ref[i][1]) ){
                rvid = window.embed.ref[i][0]
                //console.log('re-using id = ', rvid)
                seen = true
                break
            }
        }

        if (!seen) {
            rvid = ID();
            window[rvid] = rv;
            window.embed.ref.push( [rvid, rv ] )
            //transmit bloat only on first access to object
            window.embed.state[""+callid ] =  rvid +"/"+ rv
        } else
            window.embed.state[""+callid ] =  rvid
    } else
        window.embed.state[""+callid ] =""+rv
    //console.log("embed_call_impl:" + window.embed.state )
}

function isCallable(value) {
    if (!value) { return false; }
    if (typeof value !== 'function' && typeof value !== 'object') { return false; }
    if (typeof value === 'function' && !value.prototype) { return true; }
    if (hasToStringTag) { return tryFunctionObject(value); }
    if (isES6ClassFn(value)) { return false; }
    var strClass = toStr.call(value);
    return strClass === fnClass || strClass === genClass;
}


function embed_call(jsdata) {
    //var jsdata = JSON.parse(jsdata);

    //always
    var callid = jsdata['id'];
    var name = jsdata['m'];
    try {
        var path = name.rsplit('.')
        var solved = []
        solved.push( window )

        while (path){
            var elem = path.shift()
            if (elem){
                var leaf = solved[ solved.length -1 ][ elem ]
                console.log( solved[ solved.length -1 ]+" -> "+ leaf)
                solved.push( leaf )
            } else break
        }
        var target = solved[ solved.length -1 ]
        var owner = solved[ solved.length -2 ]

        if (!isCallable(target)) {
            console.log("embed_call(query="+name+") == "+target)
            window.embed.state[""+callid ] = ""+target;
            return;
        }

        //only if method call
        var params = jsdata['a'];
        var env = jsdata['k'] || {};

        console.log('embed_call:'+target +' call '+callid+' launched with',params,' on object ' +owner)

        setTimeout( embed_call_impl ,1, callid, target, owner, params );
    } catch (x) {
        console.log('malformed RPC '+jsdata+" : "+x )
    }
}


function log(msg) {
    document.getElementById('log').textContent += msg + '\n'
}
