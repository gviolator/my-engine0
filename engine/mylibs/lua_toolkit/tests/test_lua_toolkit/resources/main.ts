
/**
 * IService1
 */
interface IService1 {
  add1(x: number, y?: number): string;
  sub1(x: number, y: number): number;
}

// lua func
declare function print(s: string): void;

// declare class IClassDescriptor<T>
// {
//   public New(): T;
// }

/**
 * Native (C++) Api
 */
declare class MyService {
  public static New(): IService1;
  public static getStaticData(): string;
}

declare class MyTestCallback {
  public static New(): MyTestCallback;

  public subscribe(callback: (value: string) => number): void;
  public notify(text: string): void;
}

type Task = Promise<void>;

declare class GameState
{
  public static waitForTime(ms: number):Task;
}

class NauComponent
{
}

interface INauBehavior
{
  init(): void;
  update(): void | Task;
  postUpdate(): void
}



class MyComponent extends NauComponent implements INauBehavior
{
  public myName: string = "default_name";
  public myField1: number = 0.0;
  
  init(): void
  {

  }

  async update(): Task
  {
    await GameState.
  }
  postUpdate(): void
  {
  }
}


/**
 * 
 */
class ServiceImpl implements IService1 {
  add1(x: number, y?: number): string {
    return `Lua: ${x} + ${y} Func1 called !`;
  }

  sub1(x: number, y: number): number {
    return x - y;
  }
}

function main(): void {
  //const service: IService1 = new ServiceImpl();
  const service: IService1 = MyService.New();

  const result: string = service.add1(10.0, 20.0);
  print(`Service add1 returns: (${result})`)

  const result2 = service.sub1(95.0, 25.0);
  print(`Service sub1 returns: (${result2})`)

  const cbTest = MyTestCallback.New();
  cbTest.subscribe(text => {
    print(`Script callback handler (${text})`);
    return 0;
  });
  cbTest.notify("text_1");

  if (result == "x" && result2 == 5)
  {
  }
}



main();