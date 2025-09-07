// lua func declaration
//import "reflect-metadata";

declare function print(this: void, s: string): void;


async function myCoro0(): Promise<string> {
    print("DO return 111");
    return "Coro 0";
}



declare function myCoroCpp(this: void): Promise<String>;
declare function waitK(this: void, ms: Number): Promise<void>;

declare function generateUid(this: void): String;

function myCoroX(): Promise<String> {
    return new Promise<string>((resolve, reject) => {
        print("Construct promise");
        //nativeCoroFactory(resolve, reject);
        resolve("ZZZZ");
        print("Leave Coro")

    });
}

/**
 * Native (C++) Api
 */
declare class MyNativeBinding {
    public static New(): MyNativeBinding;

    public getKeyboardButtonPressed(key: string): boolean;
    public spawn(x: number, y: number, z: number): void;
}


interface IGameManager {


}

type NativePromiseEntry =
{
    readonly id: String;
    resolveFn: (val: any) => void;
    rejectFn: (val: any) => void;
}

type NativePromiseResult =
{
    readonly id: String;
    readonly success: Boolean;
    readonly val: any;
}

class NativePromiseHelper {
    entries: NativePromiseEntry[] = [];
    results: NativePromiseResult[] = [];


    public registerPromise(id: String, resolve: (val: any) => void, reject: (err: any) => void): void
    {
        this.entries.push({ id, resolveFn: resolve, rejectFn: reject });
    }

    public update()
    {
        if (this.results.length == 0) {
            return;
        }

        for (let res of this.results)
        {
            const id = res.id;
            const index = this.entries.findIndex(e =>
            {
                return e.id == id;
            });

            if (index >= 0) {
                print(`Complete promise (${res.id}) at (${index}) with success: ${res.success}`);
                const entry = this.entries[index];
                this.entries.splice(index, 1);
                if (res.success === true) {
                    entry.resolveFn(res.val);
                }
                else {
                    entry.rejectFn(res.val);
                }
            }
            else
            {
                print(`promise (${res.id}) NOT FOUND`);
            }
        }

        this.results.splice(0, this.results.length);
    }

    public resolvePromise(id: String, success: Boolean, val: any)
    {
        print(`Resolving prom (${id})`);
        this.results.push({ id, success, val });
    }
}

class GameManager implements IGameManager {

    private mFrameNo = 0;

    public readonly natPromiseHelper = new NativePromiseHelper();

    public init(): void {
        print("TS Game Manager initialization");
    }

    public gameStep(dt: Number): void {
        ++this.mFrameNo;
        //print(`Game step (${dt}), frame (${this.frameNo})`);
        this.natPromiseHelper.update();
    }

    public get frameNo()
    {
        return this.mFrameNo;
    }
}

let g_GameManager = new GameManager();


function setupNativePromise(this: void): {promise: Promise<void>, id: String} {

    const id = generateUid();

    const promise = new Promise((resolve: (val: any) => void, reject: (err: any) => void) => {
        g_GameManager.natPromiseHelper.registerPromise(id, resolve, reject);

    });

    return {promise, id};
}

function finalizeNativePromise(this: void, id: String, success: Boolean, val: any): void
{
    g_GameManager.natPromiseHelper.resolvePromise(id, success, val);
}


// Welcome to the TypeScript Playground, this is a website
// which gives you a chance to write, share and learn TypeScript.

// You could think of it in three ways:
//
//  - A location to learn TypeScript where nothing can break
//  - A place to experiment with TypeScript syntax, and share the URLs with others
//  - A sandbox to experiment with different compiler features of TypeScript

// function print(...args: any[]): void
// {
//     -- console.log(...args);
// }


function methodDec<This, Args extends any[], Return>(
    target: (this: This, ...args: Args) => Return,
    context: ClassMethodDecoratorContext<This, (this: This, ...args: Args) => Return>
) {
    print(`Init Decor for method (${String(context.name)})`);
     return function(this: This, ...args: Args): Return {
          const res = target.call(this, ...args);
         return res;
    }
}


// function Foo(text:)
// {

// }


function ClassDecor<This, Args extends any[]>(
    target: new (...args: Args) => This,
    context: ClassDecoratorContext<new (...args: Args) => This>)
{
    print(`My Class Decorator ${context.name}`);
}

function FieldDecor<This> (
    target: undefined,
    context: ClassFieldDecoratorContext<This, any>)
{
    print(`Grab over the field (${String(context.name)}) in ${target} `);

    
    return function(this: any, vvv: any)
    {
        

        return vvv;
    }
}


// function setDesc<This, Return>(
//     target: (t: This, arg: any) => Return,
//     context: ClassSetterDecoratorContext
// ) {

//     context.addInitializer(function(x: any)
//     {

//     });

//      //print(`Init method  (${String(context.kind)}) on (${target})`);
//      return function (this: This, arg: any): Return
//      {
//          return target.call(this, arg);
//      }
// }

function first() {
  console.log("first(): factory evaluated");
  return function (target: any, propertyKey: string, descriptor: PropertyDescriptor) {
    console.log("first(): called");
  };
}

function scnd (target: any, propertyKey: string, descriptor: PropertyDescriptor) {
    console.log("first(): called");
  };

function configurable(value: boolean) {
  return function (target: any, propertyKey: string, descriptor: PropertyDescriptor) {
    
    if (target["MY_META"] === null)
    {
        target["MY_META"] = {};
    }

    target["MY_META"] = propertyKey;
    
    //descriptor.

    print(`DDD (${target}, prop: (${propertyKey}), desc: ()`);
  };
}


// To learn more about the language, click above in "Examples" or "What's New".
// Otherwise, get started by removing these comments and the world is your playground.

//@ClassDecor
class MyComponent
{
    //@scnd
    iii: Number = 0;

    
    private mValue: string = "xxx";

    constructor(vv: string)
    {
        this.mValue = vv;
    }

    //@methodDec
    greet(): void
    {
        print(`Okay, hello ! I'm (${this.mValue})`);
    }

    //@setDesc
    @configurable(true)
    set Value(value: string)
    {
        this.mValue = value;
    }
}





function gameInit() {
    g_GameManager.init();
    const comp = new MyComponent("The name");
    


    print(`PROTO = ${(comp as any)["constructor"]["prototype"]["MY_META"]}`);

    

    let x = async (): Promise<string> => {

        let str0 = await myCoro0();
        //const str0 = 'await myCoro0();'

        print("After myCoro0");

        print("before c++ coro call");
        let str1 = await myCoroCpp();
        print("After c++ coro call");



        //let str1 = await myCoroX();
        //let str1 = "After";
        return `${str0} -> ${str1}`;
    };
/*
    const z = x();

    z.then((text: String) => {
        //print(debug.traceback());
        print(`Coro completed:${text}`);
    })
    .catch((reason) => {
        print(`Coro failed with (${reason})`);
    });
*/
}

function gameMain(dt: Number): void {
    //print("Enter Game Step");
    g_GameManager.gameStep(dt);

    if (g_GameManager.frameNo == 5)
    {
        print("Do again");

        let x = (async (): Promise<void> =>
        {
            print("WAITS -------------------");
            for (let i = 0; i < 3; ++i)
            {
                print(`waitK with ${i}`);
                await waitK(1000);
            }
            //await myCoroCpp();
            print(" ------------------- WAITS DONE");

        })();

        x.then(() =>
        {
            print("Await completed");
        });
    }


    //print("LEave Game Step");
}

//main();

/*
const binding = MyNativeBinding.New();

function globalFunction(dt: number): String {

  if (binding.getKeyboardButtonPressed("A")) {
    binding.spawn(10, 20, 30);
    print("Key pressed");

  }

  return `no press (${dt})`
}
*/