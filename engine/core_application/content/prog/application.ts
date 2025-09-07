//import * as app from 'decl';

class MyApplication implements IMyApplication {
    mTimeAccum = 0;

    register(): void {
    }

    step(dt: number): boolean {
        
        this.mTimeAccum += dt;
        print(`[Embedded logic App]: step dt:(${dt})sec, passed:(${this.mTimeAccum})sec`);
        const ret = this.mTimeAccum <= 2.0;
        print(`Will returns: (${ret})`);

        return ret;
    }
}

const g_application = new MyApplication();

declare function print(this: void, ...args: any[]): void;

export function getApplication(): IMyApplication
{
    return g_application;
}



// function interop_appMainStep1(this: void, dt: Number): Boolean {

//     return false;
// }