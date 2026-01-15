//import type * as My from 'my';
import {} from './interop';

class MyApplication implements IMyApplication {
    mTimeAccum = 0;

    register(): void {
    }

    step(dt: number): boolean {
        
        this.mTimeAccum += dt;
        mylog(`[Embedded logic App]: step dt:(${dt})sec, passed:(${this.mTimeAccum})sec`);
        const ret = this.mTimeAccum <= 2.0;
        mylog(`Will returns: (${ret})`);

        return ret;
    }
}

const g_application = new MyApplication();



export function getApplication(): IMyApplication
{
    return g_application;
}



// function interop_appMainStep1(this: void, dt: Number): Boolean {

//     return false;
// }