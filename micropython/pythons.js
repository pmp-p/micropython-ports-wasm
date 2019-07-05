"use strict";

// will hold embed I/O , stdin and stdout buffers.

window.plink = {}


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

function register(fn,fn_dn){
    if ( undef(fn_dn) )
        fn_dn = fn.name;
    //console.log('  |-- added ' + fn_dn );
    window[fn_dn]=fn;
}

//cyclic dep
window.register = register
register(undef)
//register(module_load)

function setdefault(n,v,o){
    if (o == null)
        o = window;

    if (undef(n,o)){
        o[n]=v;
        console.log('  |-- ['+n+'] set to ['+ o[n]+']' );
        return true;
    }
    return false
}

setdefault('JSDIR','')


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
register(include)



function _until(fn_solver){
    return async function fwrapper(){
        var argv = Array.from(arguments)
        function solve_me(){return  fn_solver.apply(window, argv ) }
        while (!await delay(0, solve_me ) )
            {};
    }
}
register(_until)



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

    var doscripts = true
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
            doscripts = false
        } else {
            console.log("error getting main.py from '"+argv0+"'")
            term_impl("Javascript : error getting main.py from '"+argv0+"'\r\n")
            //TODO: global control var to skip page scripts
        }

    }

    if (doscripts) {

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
          // on binary XHR
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

function wasm_file_open(url, cachefile){
    var dirpath = ""
    if ( url == cachefile ) {
        //we need to build the target path, it could be a module import.

        //transform to relative path to /
        while (cachefile.startswith("/"))
            cachefile = cachefile.substring(1)

        while (url.startswith("/"))
            url = url.substring(1)

        // is it still a path with at least a one char folder ?
        if (cachefile.indexOf('/')>0) {
            var path = cachefile.split('/')

            // last elem is the filename
            while (path.length>1) {
                var current_folder = path.shift()
                try {
                    FS.createFolder(dirpath, current_folder, true, true)
                    //FS.createPath('/', dirname, true, true)
                } catch (err) {
                    if (err.code !== 'EEXIST') throw err
                }
                dirpath = dirpath + "/" + current_folder
            }
            console.log("+dir: "+dirpath+" +file: " + path.shift())
        } else {
            // this is a root folder, abort
            if (url.indexOf(".") <1 )
                return -1
        }
        cachefile = "/" + url
        console.log("in /  +" + cachefile)
    }

    try {
        if (url[0]==":")
            url = url.substr(1)
        else {
            if (url.includes('/wyz.fr/'))
                url = CORS_BROKER + url
        }

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
        console.log("wasm_file_open :"+x)
        return -1
    }
}


window.USE_DIR_INDEX = "/index.html"

function wasm_file_exists(url, need_dot) {

    function url_exists(url,code) {
        var xhr = new XMLHttpRequest()
        xhr.open('HEAD', url, false)
        xhr.send()
        if (xhr.status == 200 )
            return code
        return -1
    }

    // we know those are all MEMFS local files.
    // and yes it's the same folder name as in another OS apps
    if (url.startswith('assets/'))
        return -1

    if (url.endswith('.mpy'))
        return -1

    // are we possibly doing folder checking ?
    if (need_dot) {
        // .mpy is blacklisted for now
        // so if it's not .py then it's a folder check.
        if (!url.endswith('.py')) {
            var found = -1

            // package search
            found = url_exists( url + '/__init__.py' , 2 )
            console.log("wasm_([dir]/file)_exists ? :"+url+ ' --> ' + '/__init__.py => '+found)
            if (found>0) return found

            //namespace search
            found = url_exists( url + USE_DIR_INDEX , 2 )
            console.log("wasm_([dir]/file)_exists ? :"+url+ ' --> ' + USE_DIR_INDEX+" => "+found)
            if (found>0) return found
        }

        // if name has no dot then it was a folder check
        console.log("wasm_(dir/[file])_exists ? :"+url)
        need_dot = url.split('.').pop()
        if (need_dot==url) {
            console.log("wasm_file_exists not-a-file :"+url)
            return -1
        }
    }

    // default is a file search
    return url_exists(url, 1)
}

// ================== uplink ===============================================

// separate file clink for cpython , plink for micropython , entry point is embed_call(struct_from_json)

// ================= STDIN =================================================
var pts = {}
   pts.i = {}
   pts.i.line = ""
   pts.i.raw = true

   pts.o = {}

plink.pts = pts
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
    window.stdin += key
}

function stdin_poll(){
    //pending draw ?
    if (stdout_blit)
        flush_stdout();

    if (!window.stdin_raw)
        return

    if (!window.stdin.length)
        return

    var utf8 = unescape(encodeURIComponent(window.stdin));
    for(var i = 0; i < utf8.length; i++) {
        window.stdin_array.push( utf8.charCodeAt(i) );
    }
    window.stdin = ""
}
setInterval( stdin_poll , 16)
window.stdin_tx =stdin_tx





function stdin_tx_chr(chr){
    console.log("stdin:control charkey:"+chr);
    window.stdin_array.push( chr );
}

// ================ STDOUT =================================================

// TODO: add a dupterm for stderr, and display error in color in xterm if not in stdin_raw mode


window.stdout_blit = false
window.stdout_array = []


if (1) {
    function flush_stdout_utf8(){
        var uint8array = new Uint8Array(window.stdout_array)
        var string = new TextDecoder().decode( uint8array )
        term_impl(string)
        window.stdout_array=[]
        stdout_blit = false
    }

    function stdout_process_utf8(cc) {
        window.stdout_array.push(cc)

        if (window.stdin_raw) {
            if (cc<128)
                stdout_blit = true
            return
        }

        if (cc==10) {
            stdout_blit = true
            return
        }
        //no blit on non raw mode until crlf
    }

    window.stdout_process = stdout_process_utf8
    window.flush_stdout = flush_stdout_utf8

}
/*
 else {

    function stdout_process_ascii(cc) {
        window.stdout_array.push( String.fromCharCode(cc) )
        stdout_blit = true
    }

    function flush_stdout_ascii(){
        term_impl(window.stdout_array.join("") )
        window.stdout_array=[]
        stdout_blit = false
    }

    window.stdout_process = stdout_process_ascii
    window.flush_stdout = flush_stdout_ascii
}
*/

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
        console.log("C-OUT ["+text+"]")
        flush_stdout()
        try {
            posix.syslog(text)
        } catch (y) {
            term_impl(text+"\r\n")
        }

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

    for(var i = 0; i < scripts.length; i++){
        var script = scripts[i]
        if(script.type == "text/µpython"){

            var emterpretURL = "micropython.binary"
            var emterpretXHR = new XMLHttpRequest;
                emterpretXHR.open("GET", emterpretURL, !0),
                emterpretXHR.responseType = "arraybuffer",
                emterpretXHR.onload = function() {
                    if (200 === emterpretXHR.status || 0 === emterpretXHR.status) {
                        Module.emterpreterFile = emterpretXHR.response
                        console.log("Starting upython via emterpreter")
                    } else {
                        console.log("Starting upython synchronously : " + emterpretXHR.status )
                    }
                    include("micropython.js")
                }
                emterpretXHR.send(null)
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
    if ( wasm_file_exists("lib/lib"+lib +".js") ){
        console.log(" =========== CAN RAW EVAL ========== ")
    }
    var lzma_file = "lib"+lib+".js.lzma"
    var blob = await get_lzma( window.lib, lib, lzma_file, size_hint, false, false)
    write_file("lib","lib"+trigger+".so",blob)
}




console.log('pythons included')


