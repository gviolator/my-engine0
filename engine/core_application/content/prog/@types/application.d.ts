
declare function print(this: void, s: string): void;


declare interface IMyApplication
{
    register(): void;
    step(dt: number): boolean;
}


declare function getApplication(): IMyApplication;
