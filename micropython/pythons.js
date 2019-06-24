"use strict";


function undef(e,o){
    if (typeof o === 'undefined' || o === null)
        o = window;
    //else console.log('domain '+o)

    try {
        e = o[e];
    } catch (x) { return true }

    if (typeof e === 'undefined' || e === null)
        return true;
    return false;
}

function defined(e,o){return !undef(e,o)}

const delay = (ms, fn_solver) => new Promise(resolve => setTimeout(() => resolve(fn_solver()), ms*1000));

String.prototype.rsplit = function(sep, maxsplit) {
    var split = this.split(sep);
    return maxsplit ? [ split.slice(0, -maxsplit).join(sep) ].concat(split.slice(-maxsplit)) : split;
}

String.prototype.endswith = String.prototype.endsWith
String.prototype.startswith = String.prototype.startsWith


function setdefault(n,v,o){
    if (o == null)
        o = window;

    if (undef(n,o)){
        o[n]=v;
        console.log('  |-- ['+n+'] set to ['+ o[n]+']' );
        return true;
    }
    return false;
}

setdefault('JSDIR','');

function include(filename, filetype){
    if (filetype===null ||typeof filetype === 'undefined')
        filetype = 'js';
        if (filename.endswith('css'))
            filetype = 'css';

    if ( (filename.indexOf('.') === 0) || (filename.indexOf('/') === 0 ) ){
        //absolute !
    } else {
        //corrected
        filename = window.JSDIR + filename;
    }

    if (filetype=="js"){ //if filename is a external JavaScript file
        var fileref=document.createElement('script')
        fileref.setAttribute("type","text/javascript")
        fileref.setAttribute("src", filename)
        fileref.setAttribute('async',false);
    }
    else if (filetype=="css"){ //if filename is an external CSS file
        var fileref=document.createElement("link")
        fileref.setAttribute("rel", "stylesheet")
        fileref.setAttribute("type", "text/css")
        fileref.setAttribute("href", filename)
    }   else {
        console.log("#error can't include "+filename+' as ' +filetype);
        return false;
    }
    if (typeof fileref!="undefined")
        console.log("#included "+filename+' as ' +filetype);

        document.getElementsByTagName("head")[0].appendChild(fileref)
        fileref.async = false;
        fileref.defer = false;
        //fileref.src = window.URL.createObjectURL( window.EMScript );
        //document.body.appendChild(fileref);
}
window.include = include;

function _until(fn_solver){
    return async function fwrapper(){
        var argv = Array.from(arguments)
        function solve_me(){return  fn_solver.apply(window, argv ) }
        while (!await delay(0, solve_me ) )
            {};
    }
}



// =================  EMSCRIPTEN ================================



function preRun(){
    console.log("preRun: Begin")
    //FS.init()
    var argv = window.location.href.split('?',2)
    var e;
    while (e=argv.shift())
        Module.arguments.push(e)
    argv = Module.arguments.pop().split('&')
    while (e=argv.shift())
        Module.arguments.push(e)
    console.log("preRun: End")
}


function write_file(dirname, filename, arraybuffer) {
    FS.createPath('/',dirname,true,true);
    FS.createDataFile(dirname,filename, arraybuffer, true, true);
}


//async
function postRun(){
    console.log("postRun: Begin")
    setTimeout(init_loop,500)
    console.log("postRun: End")
}

function PyRun_VerySimpleFile(text){
    var cs = allocate(intArrayFromString(text), 'i8', ALLOC_STACK);
    //console.log(script.text)
    Module._PyRun_VerySimpleFile(cs)
    Module._free(cs)
}

function PyRun_SimpleString(text){
    var cs = allocate(intArrayFromString(text), 'i8', ALLOC_STACK);
    //console.log(script.text)
    Module._PyRun_SimpleString(cs)
    try {
        Module._free(cs)
    } catch (x) {
            console.log('_free(): do not forget to turn off debugging')
    }
}



function init_loop(){

    console.log("init_loop:Begin (" + Module.arguments.length+")")
    var scripts = document.getElementsByTagName('script')

    if (Module.arguments.length>1) {

        var argv0 = ""+Module.arguments[1]
        if (1)
            if (argv0.startswith('http')) {
                argv0 = CORS_BROKER+argv0
            }
        else console.log("CORS PATCH OFF")
        console.log('running with sys.argv', argv0)

        window.currentTransferSize = 0
        window.currentTransfer = argv0

        var ab = awfull_get(argv0)
        if (window.currentTransferSize>=0) {
            console.log(ab.length)
            FS.createDataFile("/",'main.py', ab, true, true);
            PyRun_VerySimpleFile('main.py')
        } else {
            console.log("an error occured getting main.py from '"+argv0+"'")
            term_impl("Javascript : error occured getting main.py from '"+argv0+"'")
        }

    } else {

        for(var i = 0; i < scripts.length; i++){
            var script = scripts[i]
            if(script.type == "text/µpython"){
                PyRun_SimpleString(script.text)
            }
        }
    }
    setTimeout(Module._repl_init, 1);
    console.log("init_loop:End")


}

// ============================== FILE I/O (sync => bad) =================================


function awfull_get(url) {
    function updateProgress (oEvent) {
      if (oEvent.lengthComputable) {
        var percentComplete = oEvent.loaded / oEvent.total;
      } else {
            // Unable to compute progress information since the total size is unknown
      }
    }

    function transferFailed(evt) {
      console.log("callfs: An error occurred while transferring the file '"+window.currentTransfer+"'");
    }

    function transferCanceled(evt) {
      console.log("callfs: transfer '"+window.currentTransfer+"' has been canceled by the user.");
    }

    var oReq = new XMLHttpRequest();

    function transferComplete(evt) {
        if (oReq.status==404){
            console.log("callfs: File not found : "+ url );
            window.currentTransferSize = -1 ;

        } else {
            window.currentTransferSize = oReq.response.length;
            console.log("callfs: Transfer is complete saving : "+window.currentTransferSize);
        }
    }

    oReq.overrideMimeType("text/plain; charset=x-user-defined");
    oReq.addEventListener("progress", updateProgress);
    oReq.addEventListener("load", transferComplete);
    oReq.addEventListener("error", transferFailed);
    oReq.addEventListener("abort", transferCanceled);
    oReq.open("GET",url ,false);
    oReq.send();
    return oReq.response
}

function hack_open(url, cachefile){
    try {
        if (url[0]==":")
            url = url.substr(1)

        var ab = awfull_get(url)
        var ret = ab.length

        window.urls.index += 1
        if (!cachefile){
            cachefile = "cache_"+window.urls.index
            ret = window.urls.index
        }
        FS.createDataFile("/", cachefile, ab, true, true);
        return ret
    } catch (x) {
        console.log("hack_open :"+x)
        return 0
    }
}


function file_exists(urlToFile, need_dot) {
    if (need_dot) {
        need_dot = urlToFile.split('.').pop()
        if (need_dot==urlToFile) {
            //console.log("file_exists not-a-file :"+urlToFile)
            return 0
        }
        //console.log("file_exists ? :"+urlToFile)
    }

    var xhr = new XMLHttpRequest()
    xhr.open('HEAD', urlToFile, false)
    xhr.send()
    var ret=0
    if (xhr.status == 200 )
        ret=1
    //console.log(ret)
    return ret
}
// ================= ulink =================================================
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



// ================= STDIN =================================================
window.stdin_array = []
window.stdin = ""
window.stdin_raw = true

function window_prompt(){
    if (window.stdin.length>0) {
        var string = window.stdin
        window.stdin = ""
        console.log("sent ["+string+"]")
        return string
    }
    return null
}

function stdin_tx(key){
    window.stdin = window.stdin + key

    if (!window.stdin_raw) {
        console.log("key:"+key);
        return ;
    }
    var utf8 = unescape(encodeURIComponent(key));
    for(var i = 0; i < utf8.length; i++) {
        window.stdin_array.push( utf8.charCodeAt(i) );
    }
}

function stdin_tx_chr(chr){
    console.log("stdin:control charkey:"+chr);
    window.stdin_array.push( chr );
}

// ================ STDOUT =================================================

// TODO: add a dupterm for stderr, and display error in color in xterm if not in stdin_raw mode


window.stdout_array = []

function flush_stdout(){
    var uint8array = new Uint8Array(window.stdout_array)
    var string = new TextDecoder().decode( uint8array )
    term_impl(string)
    window.stdout_array=[]
}


function stdout_process(cc) {

    window.stdout_array.push(cc)

    if (window.stdin_raw) {
        if (cc<128)
            flush_stdout()
        return
    }
    if (cc==10) flush_stdout()
}


// this is a demultiplexer for stdout and os (DOM/js ...) control
function pts_decode(text){

    try {
        var jsdata = JSON.parse(text);
        for (key in jsdata) {
            // TODO: multiple fds for pty.js
            if (key=="1") {
                stdout_process( jsdata[key] )
                continue
            }
            if (key=="id") {
                embed_call(jsdata)
                continue
            }
            console.log("muliplexer noise : "+ key+ " = " + jsdata[key] )
        }

    } catch (x) {
        // found a raw C string via libc
        console.log("C-STR:"+x+":"+text)
        flush_stdout()
        term_impl(text+"\r\n")
    }
}

// ========================== startup hooks ======================================

window.Module = {
    preRun : [preRun],
    postRun: [postRun],
    print : pts_decode,
    printErr : console.log,
}



async function pythons(argc, argv){
    var scripts = document.getElementsByTagName('script')
    /*
    for ?0=xxxxx&1=xxxx argv style
    var argv = []
    for (var i=0;i<10;i++) {
        var arg = new URL(window.location.href).searchParams.get(i);
        if (arg) {
            console.log("argv["+i+"]=",arg)
            argv.push(arg)
        }
        else break
    }
    if (argv.length>0) {
        console.log('running with sys.argv',argv)
        //Module.arguments = argv
        include("micropython.js")
        return;
    }
    */
    for(var i = 0; i < scripts.length; i++){
        var script = scripts[i]
        if(script.type == "text/µpython"){
            console.log("starting upython")
            include("micropython.js")
            break
        }
    }
}


if ( undef("CORS_BROKER") ){
    window.CORS_BROKER = "https://cors-anywhere.herokuapp.com/"
    console.log("using default brooker CORS_BROKER="+CORS_BROKER)
}



// ========================== C =============================
window.lib = {"name":"lib"};
window.urls = {"name":"http","index":-1}

async function _get(url,trigger){
    fetch(url).then( function(r) { return r.arrayBuffer(); } ).then( function(udata) { window[trigger] = udata } );
    await _until(defined)(trigger,window.urls)
    return window.urls[trigger]
}

async function dlopen_lzma(lib,size_hint) {
    if ( file_exists("lib/lib"+lib +".js") ){
        console.log(" =========== CAN RAW EVAL ========== ")
    }
    var lzma_file = "lib"+lib+".js.lzma"
    var blob = await get_lzma( window.lib, lib, lzma_file, size_hint, false, false)
    write_file("lib","lib"+trigger+".so",blob)
}




console.log('pythons included')


