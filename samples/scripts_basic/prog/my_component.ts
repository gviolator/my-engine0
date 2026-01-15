import * as Core from '@core/application'
import {BlackBox} from '@core/some_file'

class MyComponent
{
}

const c = new MyComponent();



export function mainStep(dt: number): boolean
{
    const f:IMyApplication | null = Core.getApplication()

    if (f != null)
    {
        const res = (f as IMyApplication).step(0);
    }

    BlackBox();


    mylog('My Game Step 1 !');
    return true;
}

