const mmap = require('mmap.js')
const fs = require("fs")
const benchmarker = require('./benchmarker')

const file = "/tmp/mmapbenchmark.bin";
const MESSAGE_SIZE=100


const MESSAGE_CONTENT= "hello server\n\0\0\0\0"
const CONNECTION_MADE_STR= "connection_made\n"

var RECV_CONTENT = Buffer.alloc(MESSAGE_SIZE);
RECV_CONTENT.write("hello client\n");

const fd = fs.openSync(file, 'r+');

const fileBuf = mmap.alloc(
    MESSAGE_SIZE,
    mmap.PROT_READ | mmap.PROT_WRITE,
    mmap.MAP_SHARED,
    fd,
    0);




if(!fileBuf.toString('utf8').startsWith(CONNECTION_MADE_STR)){
    console.log("no server connection");
    return
}

function send_message(){
    fileBuf.write(MESSAGE_CONTENT);
   
}

function recv_message(){
    do{
    
    }
    while(Buffer.compare(fileBuf,RECV_CONTENT)!==0)
      
}
//benchmarker.BENCHMARK_ITERATIONS
for(i=0;i<10000;i++){
    benchmarker.startClock();
    send_message();
    recv_message();    
    benchmarker.stopClock();
}

    benchmarker.printResult(i);




fs.closeSync(fd);


