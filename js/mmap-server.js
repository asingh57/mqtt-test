const mmap = require('mmap.js')


const fs = require("fs")
const benchmarker = require('./benchmarker')

const file = "/tmp/mmapbenchmark.bin";
const MESSAGE_SIZE=100


const MESSAGE_CONTENT= "hello client\n"
const CONNECTION_MADE_STR= "connection_made\n"
//const SEEK_SET=0;


var START_BUFF = Buffer.alloc(MESSAGE_SIZE);
START_BUFF.write(CONNECTION_MADE_STR);


var RECV_CONTENT = Buffer.alloc(MESSAGE_SIZE);
RECV_CONTENT.write("hello server\n");


var fd = fs.openSync(file, 'w+');


fs.writeSync(fd, START_BUFF, 0, MESSAGE_SIZE);



const fileBuf = mmap.alloc(
    MESSAGE_SIZE,
    mmap.PROT_READ | mmap.PROT_WRITE,
    mmap.MAP_SHARED,
    fd,
    0);







function send_message(){
    fileBuf.write(MESSAGE_CONTENT);
   
}

function recv_message(){
    do{

    }
    while(Buffer.compare(fileBuf,RECV_CONTENT)!==0)
      
}
//benchmarker.BENCHMARK_ITERATIONS



fileBuf.write(CONNECTION_MADE_STR);



for(i=0;i<10000;i++){
    recv_message();
    send_message();
}






fs.closeSync(fd);



