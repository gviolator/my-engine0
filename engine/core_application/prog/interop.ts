import { getApplication } from "./application";


export function interop_appMainStep(this: void, dt: number): Boolean {

    return getApplication().step(dt);
}




