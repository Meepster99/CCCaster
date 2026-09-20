

// ugh god 


// frida -n MBAA.exe -l fridaTest.js
// just type exit ig. i wish there was a better way

//const kernel32 = Process.getModuleByName("kernel32.dll");
//let qpcAddr = kernel32.getExportByName('QueryPerformanceCounter');
//var qpcFunc = new NativeFunction(qpcAddr, 'bool', ['pointer'], {abi: 'stdcall'});

const kernel32 = Process.getModuleByName("kernel32.dll");
const qpcAddr = kernel32.getExportByName("QueryPerformanceCounter");
const qpcFunc = qpcAddr; 

let counterData = new UInt64(0);
let counterPointer = new NativePointer(counterData);

function getTime() {
    console.log("a  ", qpcFunc.toString());
    qpcFunc(counterPointer);
    return counterData;
    //return Process.getCurrentTime();
}

class Timer {

    constructor(addr, name) {

        this.name = name;
        this.addr = addr;
        this.count = 0;
        this.time = 0;

        Interceptor.attach(ptr(addr), {
            onEnter() {
                this.count += 1;
                this.startTime = getTime();
            }, 
            onLeave() {
                this.time += getTime() - this.startTime;
            }
        });

    }

}


let timers = [
    new Timer(0x0048e0a0, "dawgNoClue")
]

let tick = 0;

function actuallyDisplayShit() {
    
    console.log('\u001b[2J')
    console.log('\u001b[H');

    for(const t of timers) {
        console.log("a ", t.name, t.time)
    }

    
}

Interceptor.attach(ptr(0x0040e390), {

    onEnter(args) {
        
        for(let t of timers) {
            t.count = 0;
            t.time = 0;
        }

        //console.log(sprintf("value = %d, address = %p", 12, 2));  
    },

    onLeave() {

        tick++;
        if(tick < 10) {
            return;
        }
        tick = 0;

        actuallyDisplayShit();

    
        
    }


});
